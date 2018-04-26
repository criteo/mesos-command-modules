#ifndef __COMMAND_HOOK_HPP__
#define __COMMAND_HOOK_HPP__

#include <string>

#include <mesos/hook.hpp>
#include <mesos/module/hook.hpp>

namespace criteo {
namespace mesos {

/**
 * Hook calling external commands to handle hook events.
 *
 * The external command is executed in a fork so that it does not threaten
 * the Mesos agent. The hook is also protected from infinite loop by killing
 * the child process after a certain amount of time if it does not exit.
 */
class CommandHook : public ::mesos::Hook
{
public:
  /*
   * Constructor
   * Each argument can be empty. In that case the corresponding hook
   * method will not be treated.
   *
   * @param runTaskLabelCommand The command used by
   * slaveRunTaskLabelDecorator method if provided.
   * @param executorEnvironmentCommand The command used by
   * slaveExecutorEnvironmentDecorator if provided.
   * @param removeExecutorCommand The command used by
   * slaveRemoveExecutorHook if provided.
   */
  CommandHook(const std::string& runTaskLabelCommand,
              const std::string& executorEnvironmentCommand,
              const std::string& removeExecutorCommand);
  virtual ~CommandHook() {}

  /*
   * Run an external command computing a list of labels to add to the executor.
   * The command will be provided with a JSON containing all the
   * information required to compute the list of labels. The output of the
   * command is a JSON that is parsed into mesos::Labels.
   *
   * @param taskInfo The information regarding the tasks.
   * @param executorInfo The information regarding the executor.
   * @param frameworkInfo The information regarding the framework.
   * @param slaveInfo The information regarding the slave.
   * @return The labels to add to the executor.
   */
  virtual Result<::mesos::Labels> slaveRunTaskLabelDecorator(
    const ::mesos::TaskInfo& taskInfo,
    const ::mesos::ExecutorInfo& executorInfo,
    const ::mesos::FrameworkInfo& frameworkInfo,
    const ::mesos::SlaveInfo& slaveInfo) override;

  /*
   * Run an external command producing a list of environment variables to
   * add to the executor. The command is provided with a JSON containing
   * all the information required to compute the list of environment
   * variables. The output of the command is a JSON that is parsed into
   * mesos::Evironment.
   *
   * @param executorInfo The information regarding the executor.
   * @return The environment variables to add to the executor.
   */
  virtual Result<::mesos::Environment> slaveExecutorEnvironmentDecorator(
    const ::mesos::ExecutorInfo& executorInfo) override;

  /*
   * Run an external command when an executor is removed from the agent.
   * The command is provided with a JSON containing all the information
   * relative to the framework and the executor.
   *
   * @param frameworkInfo The information regarding the framework.
   * @param executorInfo The information regarding the executor.
   */
  virtual Try<Nothing> slaveRemoveExecutorHook(
    const ::mesos::FrameworkInfo& frameworkInfo,
    const ::mesos::ExecutorInfo& executorInfo) override;

  inline const std::string& runTaskLabelCommand() const {
    return m_runTaskLabelCommand;
  }

  inline const std::string& executorEnvironmentCommand() const {
    return m_executorEnvironmentCommand;
  }

  inline const std::string& removeExecutorCommand() const {
    return m_removeExecutorCommand;
  }

private:
  std::string m_runTaskLabelCommand;
  std::string m_executorEnvironmentCommand;
  std::string m_removeExecutorCommand;
};

}
}

#endif
