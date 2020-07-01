#ifndef __RUNNING_CONTEXT_HPP__
#define __RUNNING_CONTEXT_HPP__

#include <stout/try.hpp>

#include "Command.hpp"
#include "Logger.hpp"

#define TEMP_FILE_TEMPLATE "/tmp/criteo-mesos-XXXXXX"

namespace criteo {
namespace mesos {

class RunningContext {
 public:
  RunningContext(bool debug, const logging::Metadata& loggingMetadata,
                 const Command& command, const std::string& input);

  void deleteContext() const;
  Try<std::string> readOutput() const;
  Try<std::string> readError() const;
  const std::vector<std::string>& get_args() const { return args; };

 private:
  /*
   * Represent a temporary file that can be either written or read from.
   */
  class TemporaryFile {
   public:
    TemporaryFile();

    /*
     * Read whole content of the temporary file.
     * @return The content of the file.
     */
    std::string readAll() const;

    /*
     * Write content to the temporary file and flush it.
     * @param content The content to write to the file.
     */
    void write(const std::string& content) const;

    inline const std::string& filepath() const;

    friend std::ostream& operator<<(std::ostream& out,
                                    const TemporaryFile& temp_file) {
      out << temp_file.m_filepath;
      return out;
    }

   private:
    std::string m_filepath;
  };

  Try<std::string> readFile(const TemporaryFile& file) const;

  bool debug;
  const logging::Metadata loggingMetadata;
  std::vector<std::string> args;

  TemporaryFile inputFile;
  TemporaryFile outputFile;
  TemporaryFile errorFile;
};

}  // namespace mesos
}  // namespace criteo
#endif  // __RUNNING_CONTEXT_HPP__
