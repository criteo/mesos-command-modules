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

#include <stout/duration.hpp>
#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include <process/collect.hpp>
#include <process/process.hpp>
#include <process/subprocess.hpp>

#define TEMP_FILE_TEMPLATE "/tmp/criteo-mesos-XXXXXX"

#define READ 0
#define WRITE 1

namespace criteo {
namespace mesos {

using std::string;
using std::vector;
using namespace std::chrono;
using namespace process;

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

  /* ~TemporaryFile() { */
  /*   if (remove(m_filepath.c_str()) != 0) */
  /*     std::cerr << "Error while deleting " << m_filepath << std::endl; */
  /* } */

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
 * @param executable Absolute path to the executed of rhe command to execute in
 * the child process.
 * @param timeout The timeout deadline in seconds before killing the
 * child process.
 */
Future<Try<bool>> runCommandWithTimeout(
    const std::string& executable, const std::vector<std::string>& args,
    unsigned long timeoutInSeconds, const logging::Metadata& loggingMetadata) {
  // FIXME: remove this
  vector<string> full_args;
  full_args.push_back(executable);
  full_args.push_back(args[0]);
  full_args.push_back(args[1]);
  full_args.push_back(args[2]);
  Try<Subprocess> command =
      subprocess(executable, full_args, Subprocess::PATH(args[0]));

  if (command.isError()) {
    string errorMessage = "Error launching external command \"" + executable +
                          "\": " + command.error();
    TASK_LOG(ERROR, loggingMetadata) << errorMessage;
    return Error(errorMessage);
  }
  Subprocess process = command.get();
  return process.status()
      .then([=](Option<int> status) -> Future<Try<bool>> {
        if (status.isNone()) {
          string errorMessage = "Error getting status for external command \"" +
                                executable + "\"";
          TASK_LOG(ERROR, loggingMetadata) << errorMessage;
          return Error(errorMessage);
        } else if (status.get() != 0) {
          if (WIFSIGNALED(status.get()) && WTERMSIG(status.get()) != 0) {
            int signalCode = WTERMSIG(status.get());
            TASK_LOG(ERROR, loggingMetadata)
                << "Failed to successfully run the command \"" << executable
                << "\", it exited with signal " << signalCode;
            return Error("Command \"" + executable + "\" exited via signal " +
                         std::to_string(signalCode) + ".");
          }
          int exitCode = WEXITSTATUS(status.get());
          TASK_LOG(ERROR, loggingMetadata)
              << "Failed to successfully run the command \"" << executable
              << "\", it failed with status " << exitCode;
          return Error("Command \"" + executable +
                       "\" exited with return code " +
                       std::to_string(exitCode) + ".");
        }
        return true;
      })
      .after(
          Seconds(timeoutInSeconds),
          [=](Future<Try<bool>> future) -> Future<Try<bool>> {
            future.discard();
            TASK_LOG(WARNING, loggingMetadata)
                << "External command took too long to exit. "
                << "Sending SIGTERM...";
            if (kill(process.pid(), SIGTERM) == -1) {
              TASK_LOG(ERROR, loggingMetadata)
                  << "Failed to send SIGTERM: " << strerror(errno);
              TASK_LOG(WARNING, loggingMetadata)
                  << "External command is still running. Sending SIGKILL...";
              if (kill(process.pid(), SIGKILL) == -1) {
                TASK_LOG(ERROR, loggingMetadata)
                    << "Failed to kill the command: " << strerror(errno);
                return Error("Command \"" + executable +
                             "\" took too long to execute and SIGKILL failed.");
              }
            }
            return Error("Command \"" + executable +
                         "\" took too long to execute.");
          });
}

CommandRunner::CommandRunner(bool debug,
                             const logging::Metadata& loggingMetadata)
    : m_debug(debug), m_loggingMetadata(loggingMetadata) {}

Future<Try<string>> CommandRunner::asyncRun(const Command& command,
                                            const std::string& input) {
  try {
    TemporaryFile inputFile;
    TemporaryFile outputFile;
    TemporaryFile errorFile;
    inputFile.write(input);

    if (m_debug) {
      TASK_LOG(INFO, m_loggingMetadata)
          << "Calling command: \"" << command.command() << "\" ("
          << command.timeout() << "s) " << inputFile.filepath() << " "
          << outputFile.filepath() << " " << errorFile.filepath();
    } else {
      TASK_LOG(INFO, m_loggingMetadata)
          << "Calling command: \"" << command.command() << "\" ("
          << command.timeout() << "s)";
    }

    vector<string> args;
    args.push_back(inputFile.filepath());
    args.push_back(outputFile.filepath());
    args.push_back(errorFile.filepath());

    return runCommandWithTimeout(command.command(), args, command.timeout(),
                                 m_loggingMetadata)
        .then([=](Try<bool> t) -> Future<Try<string>> {
          if (t.isError()) {
            Try<string> stderr = os::read(errorFile.filepath());
            if (stderr.isError() || stderr.get().empty())
              return Error(t.error());
            return Error(t.error() + " Cause: " + stderr.get());
          }
          return os::read(outputFile.filepath());
        });
  } catch (const std::runtime_error& e) {
    if (m_debug) {
      return Error("[DEBUG] " + string(e.what()) + ". Input was \"" + input +
                   "\"");
    }
    return Error(e.what());
  }
}

Try<string> CommandRunner::run(const Command& command,
                               const std::string& input) {
  Future<Try<string>> output = asyncRun(command, input);
  auto result = await(output);
  if (!result.await()) {
    return Error("command timed out");
  }
  if (!output.isReady()) {
    return Error("command execution error");
  }
  return output.get();
}
}  // namespace mesos
}  // namespace criteo
