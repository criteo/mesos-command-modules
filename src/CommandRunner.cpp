#include "CommandRunner.hpp"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include <glog/logging.h>

#include <stout/nothing.hpp>
#include <stout/try.hpp>

#define TEMP_FILE_TEMPLATE "/tmp/criteo-mesos-XXXXXX"

#define READ 0
#define WRITE 1

namespace criteo {
namespace mesos {

using std::string;
using std::vector;
using namespace std::chrono;

/*
 * Represent a temporary file that can be either written or read from.
 */
class TemporaryFile {
 public:
  TemporaryFile() {
    char filepath[] = TEMP_FILE_TEMPLATE;
    int fd = mkstemp(filepath);
    if (fd == -1)
      throw std::runtime_error(
          "Unable to create temporary file to run commands");
    close(fd);
    m_filepath = std::string(filepath);
  }

  /*
   * Read whole content of the temporary file.
   * @return The content of the file.
   */
  std::string readAll() const {
    std::ifstream ifs(m_filepath);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    ifs.close();
    return content;
  }

  /*
   * Write content to the temporary file and flush it.
   * @param content The content to write to the file.
   */
  void write(const std::string& content) const {
    std::ofstream ofs;
    ofs.open(m_filepath);
    ofs << content;
    std::flush(ofs);
    ofs.close();
  }

  ~TemporaryFile() {
    if (remove(m_filepath.c_str()) != 0)
      std::cerr << "Error while deleting " << m_filepath << std::endl;
  }

  inline const std::string& filepath() const { return m_filepath; }

 private:
  std::string m_filepath;
};

inline bool fileExists(const std::string& name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

inline bool isFileExecutable(const std::string& name) {
  return !access(name.c_str(), X_OK);
}

/*
 * Fork and run a command in the child process.
 *
 * This implementation has been inspired by
 * https://dzone.com/articles/simple-popen2-implementation
 *
 * @param command The command to run in the child process.
 * @param infp The input FILE pointer to receive data from child.
 * @param outfp The output FILE pointer to send data to child.
 * @return The pid of the child process.
 */
pid_t popen2(const std::string& command, const std::vector<std::string>& args,
             int* infp = nullptr, int* outfp = nullptr) {
  int p_stdin[2], p_stdout[2];
  pid_t pid;
  if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0) {
    // TODO: avoid leaking fd that have possibly be opened
    return -1;
  }

  pid = fork();
  if (pid < 0) {
    close(p_stdin[WRITE]);
    close(p_stdin[READ]);
    close(p_stdout[WRITE]);
    close(p_stdout[READ]);
    return pid;
  } else if (pid == 0)  // executed in child
  {
    std::vector<char*> nullTerminatedArgs;
    nullTerminatedArgs.push_back(const_cast<char*>(command.c_str()));
    std::transform(
        args.begin(), args.end(), std::back_inserter(nullTerminatedArgs),
        [](const std::string& arg) { return const_cast<char*>(arg.c_str()); });
    nullTerminatedArgs.push_back(nullptr);

    close(p_stdin[WRITE]);
    dup2(p_stdin[READ], READ);
    close(p_stdout[READ]);
    dup2(p_stdout[WRITE], WRITE);
    execv(command.c_str(), &nullTerminatedArgs[0]);

    // TODO: properly close relevant fd here

    // This code will only be reached if execl fails according to the
    // documentation: https://linux.die.net/man/3/execl
    LOG(ERROR) << "Error when executing command \"" << command
               << "\": " << strerror(errno);
    exit(1);
  }

  // executed in parent
  if (infp == nullptr) {
    close(p_stdin[WRITE]);
  } else {
    *infp = p_stdin[WRITE];
  }
  close(p_stdin[READ]);
  if (outfp == nullptr) {
    close(p_stdout[READ]);
  } else {
    *outfp = p_stdout[READ];
  }
  close(p_stdout[WRITE]);
  return pid;
}

class Timer {
 public:
  Timer(const milliseconds tickPeriod, const milliseconds timeoutPeriod)
      : m_tickPeriod(tickPeriod), m_timeoutPeriod(timeoutPeriod) {}

  void run(std::function<bool(void)> onTimeoutCallback) {
    auto start = std::chrono::steady_clock::now();
    auto timeout = start + steady_clock::duration(m_timeoutPeriod);
    auto current = start;
    bool finished = false;
    m_hasTimedOut = false;

    while (!finished && current < timeout) {
      std::this_thread::sleep_for(m_tickPeriod);
      finished = onTimeoutCallback();
      current = std::chrono::steady_clock::now();
    }

    if (!finished) {
      m_hasTimedOut = true;
    }
  }

  bool hasTimedOut() const { return m_hasTimedOut; }

 private:
  const milliseconds m_tickPeriod;
  const milliseconds m_timeoutPeriod;
  bool m_hasTimedOut;
};

/*
 * Fork the process to run command and kill the child if it does not
 * finish before the timeout deadline.
 *
 * TODO(clems4ever): split this method so that it becomes easier to read.
 *
 * @param command The command to execute in the child process.
 * @param timeout The timeout deadline in seconds before killing the
 * child process.
 */
Try<Nothing> runCommandWithTimeout(const std::string& command,
                                   const std::vector<std::string>& args,
                                   unsigned long timeoutInSeconds,
                                   const logging::Metadata& loggingMetadata) {
  int status;
  bool forceKillRequired = false;
  int exitCode = 0;
  int signalCode = 0;

  if (!fileExists(command)) {
    return Error("No such file or directory: \"" + command + "\"");
  }

  if (!isFileExecutable(command)) {
    return Error("File is not executable: \"" + command + "\"");
  }

  pid_t pid = popen2(command, args);

  auto waitProcess = [pid, &status, loggingMetadata, command,
                      &forceKillRequired, &exitCode, &signalCode]() {
    int rc = waitpid(pid, &status, WNOHANG);
    if (rc < 0) {
      TASK_LOG(ERROR, loggingMetadata)
          << "Error when waiting for child process running the "
          << "external command: " << strerror(errno);
      forceKillRequired = true;
      return true;
    }

    if (rc > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
      if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        exitCode = WEXITSTATUS(status);
        TASK_LOG(ERROR, loggingMetadata)
            << "Failed to successfully run the command \"" << command
            << "\", it failed with status " + std::to_string(exitCode);
      }
      if (WIFSIGNALED(status) && WTERMSIG(status) != 0) {
        signalCode = WTERMSIG(status);
        TASK_LOG(ERROR, loggingMetadata)
            << "Failed to successfully run the command \"" << command
            << "\", it exited with signal " + std::to_string(signalCode);
      }
      return true;
    }
    return false;
  };

  milliseconds tickPeriod(100);
  Timer t1(tickPeriod, milliseconds(timeoutInSeconds * 1000));
  t1.run(waitProcess);

  if (t1.hasTimedOut() && !forceKillRequired) {
    TASK_LOG(WARNING, loggingMetadata)
        << "External command took too long to exit. "
        << "Sending SIGTERM...";
    if (kill(pid, SIGTERM) == -1) {
      TASK_LOG(ERROR, loggingMetadata)
          << "Failed to send SIGTERM: " << strerror(errno);
      forceKillRequired = true;
    } else {
      Timer t(tickPeriod, milliseconds(1000));
      t.run(waitProcess);
      if (t.hasTimedOut()) {
        forceKillRequired = true;
      }
    }
  }

  if (forceKillRequired) {
    TASK_LOG(WARNING, loggingMetadata)
        << "External command is still running. Sending SIGKILL...";
    if (kill(pid, SIGKILL) == -1) {
      TASK_LOG(ERROR, loggingMetadata)
          << "Failed to kill the command: " << strerror(errno);
      return Error("Command \"" + command +
                   "\" took too long to execute and SIGKILL failed.");
    } else {
      return Error("Command \"" + command + "\" took too long to execute.");
    }
  }

  if (exitCode > 0) {
    return Error("Command \"" + command + "\" exited with return code " +
                 std::to_string(exitCode) + ".");
  } else if (signalCode > 0) {
    return Error("Command \"" + command + "\" exited via signal " +
                 std::to_string(signalCode) + ".");
  }
  return Nothing();
}

CommandRunner::CommandRunner(bool debug,
                             const logging::Metadata& loggingMetadata)
    : m_debug(debug), m_loggingMetadata(loggingMetadata) {}

/*
 * Run a command to which we pass input and return an output. To pass
 * input and get output, CommandRunner pass two temporary files to the
 * command arguments, one being the input file the command read its
 * inputs from and one being the output file where the command must send
 * its outputs
 *
 * @param command The command to run.
 * @param input The input to pass to the command.
 * @param output The output generated by the command.
 * @param timeout The timeout deadline before killing the child process.
 * @param debug Enable debug mode logging inputs and outputs.
 *
 * @return The output of the command read from the output file.
 */
Try<std::string> CommandRunner::run(const Command& command,
                                    const std::string& input) {
  try {
    TemporaryFile inputFile;
    TemporaryFile outputFile;
    inputFile.write(input);

    if (m_debug) {
      TASK_LOG(INFO, m_loggingMetadata)
          << "Calling command: \"" << command.command() << "\" ("
          << command.timeout() << "s) " << inputFile.filepath() << " "
          << outputFile.filepath();
    } else {
      TASK_LOG(INFO, m_loggingMetadata)
          << "Calling command: \"" << command.command() << "\" ("
          << command.timeout() << "s)";
    }

    vector<string> args;
    args.push_back(inputFile.filepath());
    args.push_back(outputFile.filepath());

    Try<Nothing> result = runCommandWithTimeout(
        command.command(), args, command.timeout(), m_loggingMetadata);
    if (result.isError()) {
      throw std::runtime_error(result.error());
    }

    return outputFile.readAll();
  } catch (const std::runtime_error& e) {
    if (m_debug) {
      return Error("[DEBUG] " + string(e.what()) + ". Input was \"" + input +
                   "\"");
    }
    return Error(e.what());
  }
}
}  // namespace mesos
}  // namespace criteo
