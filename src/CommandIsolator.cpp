#include "CommandIsolator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

namespace criteo {
namespace mesos {

using std::string;

using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;

using process::Failure;

CommandIsolator::CommandIsolator(
  const string& prepareCommand,
  const string& cleanupCommand)
  : m_prepareCommand(prepareCommand),
    m_cleanupCommand(cleanupCommand)
{
}

process::Future<Option<ContainerLaunchInfo>> CommandIsolator::prepare(
  const ContainerID& containerId,
  const ContainerConfig& containerConfig) {
  if(m_prepareCommand.empty()) return None();

  LOG(INFO) << "prepare: calling command \"" << m_prepareCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  inputsJson.values["container_config"] = JSON::protobuf(containerConfig);
  auto output = CommandRunner::run(
    m_prepareCommand, stringify(inputsJson));

  if(output.empty()) return None();

  Result<ContainerLaunchInfo> containerLaunchInfo =
    jsonToProtobuf<ContainerLaunchInfo>(output);

  if(containerLaunchInfo.isError()) {
    return Failure("Unable to deserialize ContainerLaunchInfo: " +
      containerLaunchInfo.error());
  }
  return containerLaunchInfo.get();
}

process::Future<Nothing> CommandIsolator::cleanup(
  const ContainerID& containerId) {
  if(m_cleanupCommand.empty()) return Nothing();

  LOG(INFO) << "cleanup: calling command \"" << m_cleanupCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  auto output = CommandRunner::run(
    m_cleanupCommand, stringify(inputsJson));

  return Nothing();
}

}
}
