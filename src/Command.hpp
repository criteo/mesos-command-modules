#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

namespace criteo {
namespace mesos {

// This is the default value of the timeout if the user does not override it
// in configuration.
const unsigned long DEFAULT_COMMAND_TIMEOUT = 30;

/**
 * @brief The Command class represents a command, i.e., a command to be run and
 * a timeout before the command is terminated.
 */
class Command {
 public:
  Command(const std::string& command)
      : m_cmd(command), m_timeout(DEFAULT_COMMAND_TIMEOUT) {}
  Command(const std::string& command, unsigned long timeout)
      : m_cmd(command), m_timeout(timeout) {}

  bool operator==(const Command& that) const {
    return m_cmd == that.m_cmd && m_timeout == that.m_timeout;
  }

  inline const std::string& command() const { return m_cmd; }
  inline unsigned long timeout() const { return m_timeout; }

  void setTimeout(const unsigned long timeout) { m_timeout = timeout; }

 private:
  std::string m_cmd;
  unsigned long m_timeout;
};

}  // namespace mesos
}  // namespace criteo

#endif
