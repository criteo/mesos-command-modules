#include "RunningContext.hpp"

#include <fstream>
#include <stout/os.hpp>

namespace criteo {
namespace mesos {

RunningContext::TemporaryFile::TemporaryFile() {
  char filepath[] = TEMP_FILE_TEMPLATE;
  int fd = mkstemp(filepath);
  if (fd == -1)
    throw std::runtime_error("Unable to create temporary file to run commands");
  close(fd);
  m_filepath = std::string(filepath);
}

std::string RunningContext::TemporaryFile::readAll() const {
  std::ifstream ifs(m_filepath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      (std::istreambuf_iterator<char>()));
  ifs.close();
  return content;
}

void RunningContext::TemporaryFile::write(const std::string& content) const {
  std::ofstream ofs;
  ofs.open(m_filepath);
  ofs << content;
  std::flush(ofs);
  ofs.close();
}

inline const std::string& RunningContext::TemporaryFile::filepath() const {
  return m_filepath;
}

RunningContext::RunningContext(bool debug,
                               const logging::Metadata& loggingMetadata,
                               const Command& command, const std::string& input)
    : debug(debug), loggingMetadata(loggingMetadata) {
  inputFile.write(input);
  args = {inputFile.filepath(), outputFile.filepath(), errorFile.filepath()};

  if (debug) {
    TASK_LOG(INFO, loggingMetadata)
        << "Calling command: \"" << command.command() << "\" ("
        << command.timeout() << "s) " << inputFile.filepath() << " "
        << outputFile.filepath() << " " << errorFile.filepath();
  }
}

void RunningContext::deleteContext() const {
  if (debug)
    TASK_LOG(INFO, loggingMetadata) << "Removing temp files " << inputFile
                                    << " " << outputFile << " " << errorFile;
  os::rm(inputFile.filepath());
  os::rm(outputFile.filepath());
  os::rm(errorFile.filepath());
}

Try<std::string> RunningContext::readOutput() const {
  return readFile(outputFile);
}

Try<std::string> RunningContext::readError() const {
  return readFile(errorFile);
}

Try<std::string> RunningContext::readFile(const TemporaryFile& file) const {
  return os::read(file.filepath());
}
}  // namespace mesos
}  // namespace criteo
