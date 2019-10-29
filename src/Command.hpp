#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

namespace criteo {
namespace mesos {

// This is the default value of the timeout if the user does not override it
// in configuration.
const unsigned long DEFAULT_COMMAND_TIMEOUT = 30;
const float DEFAULT_COMMAND_FREQUENCE = 30;

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

class RecurrentCommand : public Command {
 public:
  explicit RecurrentCommand(const Command& command)
      : Command(command), m_frequence(DEFAULT_COMMAND_FREQUENCE) {}

  explicit RecurrentCommand(const std::string& command)
      : Command(command), m_frequence(DEFAULT_COMMAND_FREQUENCE) {}

  RecurrentCommand(const std::string& command, unsigned long timeout)
      : Command(command, timeout), m_frequence(DEFAULT_COMMAND_FREQUENCE) {}

  RecurrentCommand(const std::string& command, unsigned long timeout,
                   float frequence)
      : Command(command, timeout), m_frequence(frequence) {}

  bool operator==(const RecurrentCommand& that) const {
    return Command::operator==(that) && m_frequence == that.m_frequence;
  }

  inline float frequence() const { return m_frequence; }

  void setFrequence(const float frequence) { m_frequence = frequence; }

 private:
  float m_frequence;
};

}  // namespace mesos
}  // namespace criteo

#endif
