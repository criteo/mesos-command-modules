#include "CommandIsolator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

#include <process/dispatch.hpp>
#include <process/process.hpp>

namespace criteo {
namespace mesos {

using std::string;

using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;

using process::Failure;

const int TIMEOUT_SECONDS = 10;

class CommandIsolatorProcess : public process::Process<CommandIsolatorProcess> {
 public:
  CommandIsolatorProcess(const string& prepareCommand,
                         const string& cleanupCommand, bool isDebugMode);

  virtual process::Future<Option<ContainerLaunchInfo>> prepare(
      const ContainerID& containerId, const ContainerConfig& containerConfig);

  virtual process::Future<Nothing> cleanup(const ContainerID& containerId);

  inline const std::string& prepareCommand() const { return m_prepareCommand; }

  inline const std::string& cleanupCommand() const { return m_cleanupCommand; }

 private:
  std::string m_prepareCommand;
  std::string m_cleanupCommand;
  bool m_isDebugMode;
};

CommandIsolatorProcess::CommandIsolatorProcess(const string& prepareCommand,
                                               const string& cleanupCommand,
                                               bool isDebugMode)
    : m_prepareCommand(prepareCommand),
      m_cleanupCommand(cleanupCommand),
      m_isDebugMode(isDebugMode) {}

process::Future<Option<ContainerLaunchInfo>> CommandIsolatorProcess::prepare(
    const ContainerID& containerId, const ContainerConfig& containerConfig) {
  if (m_prepareCommand.empty()) {
    return None();
  }

  LOG(INFO) << "prepare: calling command \"" << m_prepareCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  inputsJson.values["container_config"] = JSON::protobuf(containerConfig);
  Try<string> output = CommandRunner::run(
      m_prepareCommand, stringify(inputsJson), TIMEOUT_SECONDS, m_isDebugMode);

  if (output.isError()) {
    return Failure(output.error());
  }

  if (output->empty()) {
    return None();
  }

  Result<ContainerLaunchInfo> containerLaunchInfo =
      jsonToProtobuf<ContainerLaunchInfo>(output.get());

  if (containerLaunchInfo.isError()) {
    return Failure("Unable to deserialize ContainerLaunchInfo: " +
                   containerLaunchInfo.error());
  }
  return containerLaunchInfo.get();
}

process::Future<Nothing> CommandIsolatorProcess::cleanup(
    const ContainerID& containerId) {
  if (m_cleanupCommand.empty()) {
    return Nothing();
  }

  LOG(INFO) << "cleanup: calling command \"" << m_cleanupCommand << "\"";

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);

  Try<string> output = CommandRunner::run(
      m_cleanupCommand, stringify(inputsJson), TIMEOUT_SECONDS, m_isDebugMode);

  if (output.isError()) {
    return Failure(output.error());
  }

  return Nothing();
}

CommandIsolator::CommandIsolator(const string& prepareCommand,
                                 const string& cleanupCommand, bool isDebugMode)
    : m_process(new CommandIsolatorProcess(prepareCommand, cleanupCommand,
                                           isDebugMode)) {
  spawn(m_process);
}

CommandIsolator::~CommandIsolator() {
  if (m_process != nullptr) {
    terminate(m_process);
    wait(m_process);
    delete m_process;
  }
}

process::Future<Option<ContainerLaunchInfo>> CommandIsolator::prepare(
    const ContainerID& containerId, const ContainerConfig& containerConfig) {
  return dispatch(m_process, &CommandIsolatorProcess::prepare, containerId,
                  containerConfig);
}

process::Future<Nothing> CommandIsolator::cleanup(
    const ContainerID& containerId) {
  return dispatch(m_process, &CommandIsolatorProcess::cleanup, containerId);
}

const string& CommandIsolator::prepareCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->prepareCommand();
}

const string& CommandIsolator::cleanupCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->cleanupCommand();
}
}
}
