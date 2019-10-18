#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

namespace criteo {
namespace mesos {

// This is the default value of the timeout if the user does not override it
// in configuration.
const unsigned long DEFAULT_COMMAND_TIMEOUT = 30;
const unsigned long DEFAULT_COMMAND_FREQUENCE = 30;

/**
 * @brief The Command class represents a command, i.e., a command to be run and
 * a timeout before the command is terminated.
 */
class Command {
 public:
  Command(const std::string& command)
      : m_cmd(command),
        m_timeout(DEFAULT_COMMAND_TIMEOUT),
        m_frequence(DEFAULT_COMMAND_FREQUENCE) {}
  Command(const std::string& command, unsigned long timeout)
      : m_cmd(command),
        m_timeout(timeout),
        m_frequence(DEFAULT_COMMAND_FREQUENCE) {}
  Command(const std::string& command, unsigned long timeout,
          unsigned long frequence)
      : m_cmd(command), m_timeout(timeout), m_frequence(frequence) {}

  bool operator==(const Command& that) const {
    return m_cmd == that.m_cmd && m_timeout == that.m_timeout &&
           m_frequence == that.m_frequence;
  }

  inline const std::string& command() const { return m_cmd; }
  inline unsigned long timeout() const { return m_timeout; }
  inline unsigned long frequence() const { return m_frequence; }

  void setTimeout(const unsigned long timeout) { m_timeout = timeout; }
  void setFrequence(const unsigned long frequence) { m_frequence = frequence; }

 private:
  std::string m_cmd;
  unsigned long m_timeout;
  unsigned long m_frequence;
};

}  // namespace mesos
}  // namespace criteo

#endif
