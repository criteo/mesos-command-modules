#ifndef __COMMAND_QOS_CONTROLLER_HPP__
#define __COMMAND_QOS_CONTROLLER_HPP__

#include <string>

#include <mesos/module/qos_controller.hpp>
#include <mesos/resources.hpp>
#include <process/process.hpp>

#include <process/future.hpp>

#include "CommandRunner.hpp"

namespace criteo {
namespace mesos {

class CommandQoSControllerProcess;

class CommandQoSController : public ::mesos::slave::QoSController {
 public:
  explicit CommandQoSController(const std::string& name,
                                const Option<Command>& CommandCorrections,
                                bool isDebugMode);

  virtual ~CommandQoSController();

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<::mesos::ResourceUsage>()>& usage);

  virtual process::Future<std::list<::mesos::slave::QoSCorrection>>
  corrections();

  inline const Option<Command>& oversubscribableCommand() const {
    return m_correctionsCommand;
  }

 private:
  const Option<Command> m_correctionsCommand;
  CommandQoSControllerProcess* process;
  bool m_isDebugMode;
};

}  // namespace criteo
}  // namespace mesos
#endif
