#include "CommandHook.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

namespace criteo {
namespace mesos {

CommandHook::CommandHook(const std::string& runTaskLabelCommand,
                         const std::string& executorEnvironmentCommand,
                         const std::string& removeExecutorCommand)
  : m_runTaskLabelCommand(runTaskLabelCommand),
    m_executorEnvironmentCommand(executorEnvironmentCommand),
    m_removeExecutorCommand(removeExecutorCommand)
{
}

Result<::mesos::Labels> CommandHook::slaveRunTaskLabelDecorator(
  const ::mesos::TaskInfo& taskInfo,
  const ::mesos::ExecutorInfo& executorInfo,
  const ::mesos::FrameworkInfo& frameworkInfo,
  const ::mesos::SlaveInfo& slaveInfo) {
  if(m_runTaskLabelCommand.empty()) return None();

  LOG(INFO) << "slaveRunTaskLabelDecorator: calling command \""
            << m_runTaskLabelCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["task_info"] = JSON::protobuf(taskInfo);
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  inputsJson.values["framework_info"] = JSON::protobuf(frameworkInfo);
  inputsJson.values["slave_info"] = JSON::protobuf(slaveInfo);
  auto output = CommandRunner::run(
    m_runTaskLabelCommand, stringify(inputsJson));
  return jsonToProtobuf<::mesos::Labels>(output);
}

Result<::mesos::Environment> CommandHook::slaveExecutorEnvironmentDecorator(
  const ::mesos::ExecutorInfo& executorInfo) {
  if(m_executorEnvironmentCommand.empty()) return None();

  LOG(INFO) << "slaveExecutorEnvironmentDecorator: calling command \""
            << m_executorEnvironmentCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  auto output = CommandRunner::run(
    m_executorEnvironmentCommand, stringify(inputsJson));
  return jsonToProtobuf<::mesos::Environment>(output);
}

Try<Nothing> CommandHook::slaveRemoveExecutorHook(
  const ::mesos::FrameworkInfo& frameworkInfo,
  const ::mesos::ExecutorInfo& executorInfo) {
  if(m_removeExecutorCommand.empty()) return Nothing();

  LOG(INFO) << "slaveRemoveExecutorHook: calling command \""
            << m_removeExecutorCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["framework_info"] = JSON::protobuf(frameworkInfo);
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  CommandRunner::run(
    m_removeExecutorCommand, stringify(inputsJson));
  return Nothing();
}
}
}
