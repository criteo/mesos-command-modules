#include "CommandRunner.hpp"
#include <memory>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#include <glog/logging.h>
#include <stout/try.hpp>

#define TEMP_FILE_TEMPLATE "/tmp/criteo-mesos-XXXXXX"

#define READ 0
#define WRITE 1

namespace criteo {
namespace mesos {
namespace CommandRunner {

/*
 * Represent a temporary file that can be either written or read from.
 */
class TemporaryFile
{
public:
  TemporaryFile() {
    char filepath[] = TEMP_FILE_TEMPLATE;
    if(mkstemp(filepath) == -1)
      throw std::runtime_error(
        "Unable to create temporary file to run commands");
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
    if(remove(m_filepath.c_str()) != 0)
      std::cerr << "Error while deleting " << m_filepath << std::endl;
  }

  inline const std::string& filepath() const { return m_filepath; }

private:
  std::string m_filepath;
};


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
             int *infp = NULL, int *outfp = NULL)
{
  int p_stdin[2], p_stdout[2];
  pid_t pid;
  if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0) {
    return -1;
  }

  pid = fork();
  if (pid < 0) {
    return pid;
  }
  else if (pid == 0) // executed in child
  {
    std::vector<char *> nullTerminatedArgs;
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

    // This code will only be reached if execl fails according to the
    // documentation: https://linux.die.net/man/3/execl
    LOG(ERROR) << "Error when executing command \"" << command << "\": " << strerror(errno);
    exit(1);
  }

  // executed in parent
  if (infp == NULL) {
    close(p_stdin[WRITE]);
  }
  else {
    *infp = p_stdin[WRITE];
  }
  if (outfp == NULL) {
    close(p_stdout[READ]);
  }
  else {
    *outfp = p_stdout[READ];
  }
  return pid;
}

/*
 * Fork the process to run command and kill the child if it does not
 * finish before the timeout deadline.
 *
 * @param command The command to execute in the child process.
 * @param timeout The timeout deadline in seconds before killing the
 * child process.
 */
void runCommandWithTimeout(const std::string& command,
  const std::vector<std::string>& args, int timeout) {
  int status;
  unsigned int tick = 0;
  const unsigned int SECONDS = 1000000000;
  const unsigned int TEN_MS = 10000000;
  unsigned long long processTicks = timeout * (SECONDS / TEN_MS);
  const unsigned long long terminationTicks = SECONDS / TEN_MS;
  unsigned long long totalTicks = processTicks + terminationTicks;
  struct timespec timeoutSpec = {0, TEN_MS};
  pid_t pid = popen2(command, args);
  bool forceKill = false;

  while (tick < totalTicks) {
    nanosleep(&timeoutSpec, NULL);
    if (tick == processTicks) {
      LOG(WARNING) << "External command took too long to exit. "
        << "Sending SIGTERM...";
      if(kill(pid, SIGTERM) == -1) {
        LOG(ERROR) << "Failed to send SIGTERM: " << strerror(errno);
        break;
      }
    }

    int rc = waitpid(pid, &status, WNOHANG);
    if (rc < 0) {
      LOG(ERROR) << "Error when waiting for child process running the "
                 << "external command: " << strerror(errno);
      forceKill = true;
      break;
    }
    if (rc > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
      break;
    }
    tick++;
  }

  if(forceKill || tick == totalTicks) {
    LOG(WARNING) << "External command is still running. Sending SIGKILL...";
    if(kill(pid, SIGKILL) == -1) {
      LOG(ERROR) << "Failed to kill the command: " << strerror(errno);
    }
  }
}

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
 * @return The output of the command read from the output file.
 */
std::string run(const std::string& command, const std::string& input,
  int timeout) {
  try {
    TemporaryFile inputFile;
    TemporaryFile outputFile;
    inputFile.write(input);

    LOG(INFO) << "Fork and execute: " << command
              << " " << inputFile.filepath()
              << " " << outputFile.filepath();
    std::vector<std::string> args;
    args.push_back(inputFile.filepath());
    args.push_back(outputFile.filepath());
    runCommandWithTimeout(command, args, timeout);
    return outputFile.readAll();
  }
  catch(const std::runtime_error& e) {
    LOG(ERROR) << e.what();
    return std::string();
  }
}

}
}
}
