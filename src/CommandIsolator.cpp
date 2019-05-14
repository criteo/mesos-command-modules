#include "CommandIsolator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

#include <glog/logging.h>
#include <process/dispatch.hpp>
#include <process/process.hpp>
#include <process/time.hpp>

namespace criteo {
namespace mesos {

using process::Clock;
using std::string;

using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;
using ::mesos::slave::ContainerLimitation;

using process::Failure;
using process::Future;

class CommandIsolatorProcess : public process::Process<CommandIsolatorProcess> {
 public:
  CommandIsolatorProcess(const Option<Command>& prepareCommand,
                         const Option<Command>& watchCommand,
                         const Option<Command>& cleanupCommand,
                         const Option<Command>& usageCommand, bool isDebugMode);

  virtual process::Future<Option<ContainerLaunchInfo>> prepare(
      const ContainerID& containerId, const ContainerConfig& containerConfig);

  virtual process::Future<ContainerLimitation> watch(
      const ContainerID& containerId);

  virtual process::Future<Nothing> cleanup(const ContainerID& containerId);

  virtual process::Future<::mesos::ResourceStatistics> usage(
      const ContainerID& containerId);

  inline const Option<Command>& prepareCommand() const {
    return m_prepareCommand;
  }

  inline const Option<Command>& cleanupCommand() const {
    return m_cleanupCommand;
  }

 private:
  inline static ::mesos::ResourceStatistics emptyStats() {
    ::mesos::ResourceStatistics stats;
    stats.set_timestamp(Clock::now().secs());
    return stats;
  }

  Option<Command> m_prepareCommand;
  Option<Command> m_watchCommand;
  Option<Command> m_cleanupCommand;
  Option<Command> m_usageCommand;
  bool m_isDebugMode;
};

CommandIsolatorProcess::CommandIsolatorProcess(
    const Option<Command>& prepareCommand, const Option<Command>& watchCommand,
    const Option<Command>& cleanupCommand, const Option<Command>& usageCommand,
    bool isDebugMode)
    : m_prepareCommand(prepareCommand),
      m_watchCommand(watchCommand),
      m_cleanupCommand(cleanupCommand),
      m_usageCommand(usageCommand),
      m_isDebugMode(isDebugMode) {}

process::Future<Option<ContainerLaunchInfo>> CommandIsolatorProcess::prepare(
    const ContainerID& containerId, const ContainerConfig& containerConfig) {
  if (m_prepareCommand.isNone()) {
    return None();
  }

  logging::Metadata metadata = {containerId.value(), "prepare"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  inputsJson.values["container_config"] = JSON::protobuf(containerConfig);

  Try<string> output = CommandRunner(m_isDebugMode, metadata)
                           .run(m_prepareCommand.get(), stringify(inputsJson));

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

process::Future<ContainerLimitation> CommandIsolatorProcess::watch(
    const ContainerID& containerId) {
  if (m_watchCommand.isNone()) {
    return process::Future<ContainerLimitation>();
  }

  logging::Metadata metadata = {containerId.value(), "watch"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);

  Try<string> output = CommandRunner(m_isDebugMode, metadata)
                           .run(m_watchCommand.get(), stringify(inputsJson));

  if (output.isError()) {
    LOG(WARNING) << "Unable to parse output: " << output.error();
    return process::Future<ContainerLimitation>();
  }

  if (output->empty()) {
    return process::Future<ContainerLimitation>();
  }

  Result<ContainerLimitation> containerLimitation =
      jsonToProtobuf<ContainerLimitation>(output.get());

  if (containerLimitation.isError()) {
    LOG(WARNING) << "Unable to deserialize ContainerLimitation: "
                 << containerLimitation.error();
    return process::Future<ContainerLimitation>();
  }
  return containerLimitation.get();
}

process::Future<::mesos::ResourceStatistics> CommandIsolatorProcess::usage(
    const ContainerID& containerId) {
  if (m_usageCommand.isNone()) return emptyStats();

  logging::Metadata metadata = {containerId.value(), "usage"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);

  return CommandRunner(m_isDebugMode, metadata)
      .asyncRun(m_usageCommand.get(), stringify(inputsJson))
      .then([](Try<string> output) -> Future<::mesos::ResourceStatistics> {
        if (output.isError()) {
          LOG(WARNING) << "Unable to parse output: " << output.error();
          return emptyStats();
        }
        if (output->empty()) {
          LOG(WARNING) << "Output is empty";
          return emptyStats();
        }
        Result<::mesos::ResourceStatistics> resourceStatistics =
            jsonToProtobuf<::mesos::ResourceStatistics>(output.get());

        if (resourceStatistics.isError()) {
          LOG(WARNING) << "Unable to deserialize ResourceStatistics: "
                       << resourceStatistics.error();
          return emptyStats();
        }
        return resourceStatistics.get();
      })
      .recover([](const Future<::mesos::ResourceStatistics>& result)
                   -> Future<::mesos::ResourceStatistics> {
        LOG(WARNING) << "Failed to run usage command: " << result.failure();
        return emptyStats();
      });
}

process::Future<Nothing> CommandIsolatorProcess::cleanup(
    const ContainerID& containerId) {
  if (m_cleanupCommand.isNone()) {
    return Nothing();
  }

  logging::Metadata metadata = {containerId.value(), "cleanup"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);

  Try<string> output = CommandRunner(m_isDebugMode, metadata)
                           .run(m_cleanupCommand.get(), stringify(inputsJson));

  if (output.isError()) {
    return Failure(output.error());
  }

  return Nothing();
}

CommandIsolator::CommandIsolator(const Option<Command>& prepareCommand,
                                 const Option<Command>& watchCommand,
                                 const Option<Command>& cleanupCommand,
                                 const Option<Command>& usageCommand,
                                 bool isDebugMode)
    : m_process(new CommandIsolatorProcess(prepareCommand, watchCommand,
                                           cleanupCommand, usageCommand,
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

process::Future<ContainerLimitation> CommandIsolator::watch(
    const ContainerID& containerId) {
  return dispatch(m_process, &CommandIsolatorProcess::watch, containerId);
}

process::Future<Nothing> CommandIsolator::cleanup(
    const ContainerID& containerId) {
  return dispatch(m_process, &CommandIsolatorProcess::cleanup, containerId);
}

process::Future<::mesos::ResourceStatistics> CommandIsolator::usage(
    const ContainerID& containerId) {
  return dispatch(m_process, &CommandIsolatorProcess::usage, containerId);
}

const Option<Command>& CommandIsolator::prepareCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->prepareCommand();
}

const Option<Command>& CommandIsolator::cleanupCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->cleanupCommand();
}
}  // namespace mesos
}  // namespace criteo
