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
#include <stout/os.hpp>
#include <stout/proc.hpp>
#include <stout/try.hpp>

#include <process/after.hpp>
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

  inline const std::string& filepath() const { return m_filepath; }

 private:
  std::string m_filepath;
};

inline static bool processStillRunning(pid_t pid) {
  return proc::status(pid).isSome();
}

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
  vector<string> commandLine = {executable, args[0], args[1], args[2]};

  Try<Subprocess> command =
      subprocess(executable, commandLine, Subprocess::PATH(args[0]));

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
          string error(os::strerror(exitCode));
          TASK_LOG(ERROR, loggingMetadata)
              << "Failed to successfully run the command \"" << executable
              << "\", it failed with status " << exitCode << " (" << error
              << ")";
          return Error("Command \"" + executable +
                       "\" exited with return code " +
                       std::to_string(exitCode) + ".");
        }
        return true;
      })
      .after(
          Seconds(timeoutInSeconds),
          [=](Future<Try<bool>> future) -> Future<Try<bool>> {
            TASK_LOG(WARNING, loggingMetadata)
                << "External command took too long to exit. "
                << "Sending SIGTERM to " << process.pid() << "...";
            Try<std::list<os::ProcessTree>> kill =
                os::killtree(process.pid(), SIGTERM);
            if (kill.isError()) {
              TASK_LOG(ERROR, loggingMetadata)
                  << "Failed to send SIGTERM: " << kill.error();
            }
            return after(Seconds(1)).then([=]() -> Future<Try<bool>> {
              if (processStillRunning(process.pid())) {
                TASK_LOG(WARNING, loggingMetadata)
                    << "External command is still running. Sending SIGKILL...";
                Try<std::list<os::ProcessTree>> kill =
                    os::killtree(process.pid(), SIGKILL);
                if (kill.isError()) {
                  TASK_LOG(ERROR, loggingMetadata)
                      << "Failed to kill the command: " << kill.error();
                  return Failure(
                      "Command \"" + executable +
                      "\" took too long to execute and SIGKILL failed.");
                }
              }
              return Failure("Command \"" + executable +
                             "\" took too long to execute.");
            });
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

    vector<string> args = {inputFile.filepath(), outputFile.filepath(),
                           errorFile.filepath()};

    return runCommandWithTimeout(command.command(), args, command.timeout(),
                                 m_loggingMetadata)
        .then([=](Try<bool> status) -> Future<Try<string>> {
          if (status.isError()) {
            Try<string> stderr = os::read(errorFile.filepath());
            if (stderr.isError() || stderr.get().empty())
              return Error(status.error());
            return Error(status.error() + " Cause: " + stderr.get());
          }
          return os::read(outputFile.filepath());
        })
        .onAny([&](Future<Try<string>> output) -> Future<Try<string>> {
          os::rm(inputFile.filepath());
          os::rm(outputFile.filepath());
          os::rm(errorFile.filepath());
          return output;
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
    return Error("Command timed out");
  }
  if (!output.isReady()) {
    return Error("Command execution error");
  }
  return output.get();
}
}  // namespace mesos
}  // namespace criteo
