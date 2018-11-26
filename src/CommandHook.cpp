#include "CommandHook.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

namespace criteo {
namespace mesos {

using std::string;

CommandHook::CommandHook(
   const Option<Command>& runTaskLabelCommand,
   const Option<Command>& executorEnvironmentCommand,
   const Option<Command>& removeExecutorCommand,
   bool isDebugMode)
    : m_runTaskLabelCommand(runTaskLabelCommand),
      m_executorEnvironmentCommand(executorEnvironmentCommand),
      m_removeExecutorCommand(removeExecutorCommand),
      m_isDebugMode(isDebugMode) {}


Result<::mesos::Labels> CommandHook::slaveRunTaskLabelDecorator(
    const ::mesos::TaskInfo& taskInfo,
    const ::mesos::ExecutorInfo& executorInfo,
    const ::mesos::FrameworkInfo& frameworkInfo,
    const ::mesos::SlaveInfo& slaveInfo) {
  if (m_runTaskLabelCommand.isNone()) {
    return None();
  }

  logging::Metadata metadata = {executorInfo.executor_id().value(), "slaveRunTaskLabelDecorator"};

  JSON::Object inputsJson;
  inputsJson.values["task_info"] = JSON::protobuf(taskInfo);
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  inputsJson.values["framework_info"] = JSON::protobuf(frameworkInfo);
  inputsJson.values["slave_info"] = JSON::protobuf(slaveInfo);
  Try<string> output =
      CommandRunner(m_isDebugMode, metadata)
          .run(m_runTaskLabelCommand.get(), stringify(inputsJson));

  if (output.isError()) {
    return Error(output.error());
  }

  return jsonToProtobuf<::mesos::Labels>(output.get());
}


Result<::mesos::Environment> CommandHook::slaveExecutorEnvironmentDecorator(
    const ::mesos::ExecutorInfo& executorInfo) {
  if (m_executorEnvironmentCommand.isNone()) {
    return None();
  }

  logging::Metadata metadata = {
    executorInfo.executor_id().value(),
    "slaveExecutorEnvironmentDecorator"
  };

  JSON::Object inputsJson;
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  Try<string> output =
      CommandRunner(m_isDebugMode, metadata)
          .run(m_executorEnvironmentCommand.get(), stringify(inputsJson));

  if (output.isError()) {
    return Error(output.error());
  }

  return jsonToProtobuf<::mesos::Environment>(output.get());
}


Try<Nothing> CommandHook::slaveRemoveExecutorHook(
    const ::mesos::FrameworkInfo& frameworkInfo,
    const ::mesos::ExecutorInfo& executorInfo) {
  if (m_removeExecutorCommand.isNone()) return Nothing();

  logging::Metadata metadata = {
    executorInfo.executor_id().value(),
    "slaveExecutorEnvironmentDecorator"
  };

  JSON::Object inputsJson;
  inputsJson.values["framework_info"] = JSON::protobuf(frameworkInfo);
  inputsJson.values["executor_info"] = JSON::protobuf(executorInfo);
  Try<string> output =
      CommandRunner(m_isDebugMode, metadata)
          .run(m_removeExecutorCommand.get(), stringify(inputsJson));

  if (output.isError()) {
    return Error(output.error());
  }

  return Nothing();
}

}
}
