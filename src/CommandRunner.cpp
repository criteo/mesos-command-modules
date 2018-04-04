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
pid_t popen2(const char *command, int *infp = NULL, int *outfp = NULL)
{
  int p_stdin[2], p_stdout[2];
  pid_t pid;
  if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
    return -1;

  pid = fork();
  if (pid < 0) {
    return pid;
  }
  else if (pid == 0) // executed in child
  {
    close(p_stdin[WRITE]);
    dup2(p_stdin[READ], READ);
    close(p_stdout[READ]);
    dup2(p_stdout[WRITE], WRITE);
    execl("/bin/sh", "sh", "-c", command, NULL);
    LOG(ERROR) << "Error when executing command \"" << command << "\"";
    exit(1);
  }

  // executed in parent
  if (infp == NULL)
    close(p_stdin[WRITE]);
  else
    *infp = p_stdin[WRITE];
  if (outfp == NULL)
    close(p_stdout[READ]);
  else
    *outfp = p_stdout[READ];
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
void runCommandWithTimeout(const std::string& command, int timeout) {
  int status;
  int times = 0;
  const int SECONDS = 1000000000;
  const int TEN_MS = 10000000;
  int max_times = timeout * (SECONDS / TEN_MS);
  struct timespec timeoutSpec = {0, TEN_MS};
  pid_t pid = popen2(command.c_str());

  while (times < max_times) {
    nanosleep(&timeoutSpec, NULL);
    int rc = waitpid(pid, &status, WNOHANG);
    if (rc < 0) {
      LOG(ERROR) << "Error when waiting for child process running the "
                 << "external command";
      return;
    }
    if (rc > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
      break;
    }
    times++;
  }

  if(times == max_times) {
    LOG(WARNING) << "External command took too long to exit. Killing it...";
    kill(pid, SIGKILL);
  }
}

/*
 * Run a command and provide it with two filepath, one containing the
 * input and one to write the output to.
 *
 * @param command The command to run.
 * @param inputFilepath The file path of the file where input is stored.
 * @param outputFilepath The file path of the file where output is stored.
 * @param timeout The timeout deadline in seconds to wait before killing
 * the process running the command.
 * @return The standard output of the command.
 */
void runCommand(const std::string& command, const std::string& inputFilepath,
                       const std::string& outputFilepath, int timeout) {
  std::stringstream fullCommand;
  fullCommand << command << " " << inputFilepath << " " << outputFilepath;
  runCommandWithTimeout(fullCommand.str(), timeout);
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
    runCommand(command, inputFile.filepath(), outputFile.filepath(), timeout);
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
