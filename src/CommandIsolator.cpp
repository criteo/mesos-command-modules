#include "CommandIsolator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

#include <glog/logging.h>
#include <process/after.hpp>
#include <process/dispatch.hpp>
#include <process/loop.hpp>
#include <process/process.hpp>
#include <process/time.hpp>
#include <stout/os/mkdir.hpp>
#include <stout/os/rm.hpp>

namespace criteo {
namespace mesos {

using process::Clock;
using std::string;

using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;
using ::mesos::slave::ContainerLimitation;
using ::mesos::slave::ContainerState;

using process::Break;
using process::Continue;
using process::ControlFlow;
using process::Failure;
using process::Future;
using process::loop;
using process::after;

const string COMMAND_ISOLATOR_STATE_DIR = "/var/run/mesos/isolators/command";

class CommandIsolatorProcess : public process::Process<CommandIsolatorProcess> {
 public:
  CommandIsolatorProcess(const string& name,
                         const Option<Command>& prepareCommand,
                         const Option<Command>& isolateCommand,
                         const Option<RecurrentCommand>& watchCommand,
                         const Option<Command>& cleanupCommand,
                         const Option<Command>& usageCommand, bool isDebugMode);

  virtual process::Future<Option<ContainerLaunchInfo>> prepare(
      const ContainerID& containerId, const ContainerConfig& containerConfig);

  virtual process::Future<Nothing> recover(
      const std::vector<ContainerState>& states,
      const hashset<ContainerID>& orphans);

  virtual process::Future<ContainerLimitation> watch(
      const ContainerID& containerId);

  // Isolate the executor.
  virtual process::Future<Nothing> isolate(const ContainerID& containerId,
                                           pid_t pid);

  virtual process::Future<Nothing> cleanup(const ContainerID& containerId);

  virtual process::Future<::mesos::ResourceStatistics> usage(
      const ContainerID& containerId);

  inline const Option<Command>& prepareCommand() const {
    return m_prepareCommand;
  }
  inline const Option<Command>& isolateCommand() const {
    return m_isolateCommand;
  }
  inline const Option<Command>& cleanupCommand() const {
    return m_cleanupCommand;
  }

  inline bool hasContainerContext(const ContainerID& containerId) {
    return m_infos.contains(containerId);
  }

 private:
  inline static ::mesos::ResourceStatistics emptyStats(
      double timestamp = Clock::now().secs()) {
    ::mesos::ResourceStatistics stats;
    stats.set_timestamp(timestamp);
    return stats;
  }

  Try<Nothing> saveContainerContext(const ContainerID& containerId,
                                    const ContainerConfig& containerConfig);
  Try<ContainerConfig> restoreContainerContext(const ContainerID& containerId);
  Try<Nothing> cleanContainerContext(const ContainerID& containerId);

  string m_name;
  Option<Command> m_prepareCommand;
  Option<Command> m_isolateCommand;
  Option<RecurrentCommand> m_watchCommand;
  Option<Command> m_cleanupCommand;
  Option<Command> m_usageCommand;
  bool m_isDebugMode;
  hashmap<ContainerID, ContainerConfig> m_infos;
};

CommandIsolatorProcess::CommandIsolatorProcess(
    const string& name, const Option<Command>& prepareCommand,
    const Option<Command>& isolateCommand,
    const Option<RecurrentCommand>& watchCommand,
    const Option<Command>& cleanupCommand, const Option<Command>& usageCommand,
    bool isDebugMode)
    : m_name(name),
      m_prepareCommand(prepareCommand),
      m_isolateCommand(isolateCommand),
      m_watchCommand(watchCommand),
      m_cleanupCommand(cleanupCommand),
      m_usageCommand(usageCommand),
      m_isDebugMode(isDebugMode) {}

Try<Nothing> CommandIsolatorProcess::saveContainerContext(
    const ContainerID& containerId, const ContainerConfig& containerConfig) {
  const string& context_dir = path::join(COMMAND_ISOLATOR_STATE_DIR, m_name);
  Result<Nothing> create_context_dir = os::mkdir(context_dir, true);
  if (create_context_dir.isError()) {
    return Error("Failed to create context directory for isolator " + m_name +
                 ": " + create_context_dir.error());
  }
  const string& context_file_path =
      path::join(context_dir, stringify(containerId));
  Result<Nothing> write_context =
      os::write(context_file_path, stringify(JSON::protobuf(containerConfig)));
  if (write_context.isError()) {
    return Error("Failed writing context file for container " +
                 stringify(containerId) + ": " + write_context.error());
  }
  return Nothing();
}

Try<ContainerConfig> CommandIsolatorProcess::restoreContainerContext(
    const ContainerID& containerId) {
  const string& context_file_path =
      path::join(COMMAND_ISOLATOR_STATE_DIR, m_name, stringify(containerId));
  Result<string> context_json = os::read(context_file_path);
  if (context_json.isError()) {
    return Error("Failed reading context file: " + context_json.error());
  }
  Result<ContainerConfig> containerConfig =
      jsonToProtobuf<ContainerConfig>(context_json.get());
  if (containerConfig.isError()) {
    return Error("Unable to deserialize ContainerConfig: " +
                 containerConfig.error());
  }
  return containerConfig.get();
}

Try<Nothing> CommandIsolatorProcess::cleanContainerContext(
    const ContainerID& containerId) {
  m_infos.erase(containerId);
  const string& context_file_path =
      path::join(COMMAND_ISOLATOR_STATE_DIR, m_name, stringify(containerId));
  return os::rm(context_file_path);
}

process::Future<Option<ContainerLaunchInfo>> CommandIsolatorProcess::prepare(
    const ContainerID& containerId, const ContainerConfig& containerConfig) {
  if (m_infos.contains(containerId)) {
    return Failure("mesos-command-module already initialized for container");
  } else {
    m_infos.put(containerId, containerConfig);
  }
  saveContainerContext(containerId, containerConfig);
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

process::Future<Nothing> CommandIsolatorProcess::isolate(
    const ContainerID& containerId, pid_t pid) {
  if (m_isolateCommand.isNone()) {
    return Nothing();
  }
  logging::Metadata metadata = {containerId.value(), "isolate"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  inputsJson.values["pid"] = pid;

  if (m_infos.contains(containerId)) {
    inputsJson.values["container_config"] =
        JSON::protobuf(m_infos[containerId]);

  } else {
    LOG(WARNING)
        << "Missing container info during isolation of container with pid"
        << pid;
  }

  Try<string> output = CommandRunner(m_isDebugMode, metadata)
                           .run(m_isolateCommand.get(), stringify(inputsJson));
  if (output.isError()) {
    return Failure(output.error());
  }
  return Nothing();
}

process::Future<Nothing> CommandIsolatorProcess::recover(
    const std::vector<ContainerState>& states,
    const hashset<ContainerID>& orphans) {
  for (const ContainerState& state : states) {
    const ContainerID& containerId = state.container_id();
    LOG(INFO) << "Trying to restore context for " << stringify(containerId);
    Result<ContainerConfig> containerConfig =
        restoreContainerContext(containerId);
    if (containerConfig.isError()) {
      LOG(ERROR) << "Can't restore context for " << stringify(containerId)
                 << ": " << containerConfig.error();
      continue;
    }
    m_infos.put(containerId, containerConfig.get());
    LOG(INFO) << "Successfully restored context for " << stringify(containerId);
  }
  return Nothing();
}

process::Future<ContainerLimitation> CommandIsolatorProcess::watch(
    const ContainerID& containerId) {
  if (m_watchCommand.isNone()) {
    return process::Future<ContainerLimitation>();
  }

  logging::Metadata metadata = {containerId.value(), "watch"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  if (m_infos.contains(containerId)) {
    inputsJson.values["container_config"] =
        JSON::protobuf(m_infos[containerId]);
  } else {
    return Failure(
        "mesos-command-module is not initialized for current container");
  }

  std::string inputStringified = stringify(inputsJson);
  RecurrentCommand command = m_watchCommand.get();
  bool isDebugMode = m_isDebugMode;

  process::UPID proc = spawn(new process::ProcessBase());

  Future<ContainerLimitation> future = loop(
      proc,
      [isDebugMode, metadata, inputStringified, command]() {
        Try<string> output = CommandRunner(isDebugMode, metadata)
                                 .runWithoutTimeout(command, inputStringified);
        return output;
      },
      [command, this, containerId](
          Try<string> output) -> Future<ControlFlow<ContainerLimitation>> {
        try {
          if (!m_infos.contains(containerId)) {
            LOG(WARNING) << "Terminating watch loop for containerId: "
                         << containerId;
            // Returning a discarded future stops the loop
            Future<ControlFlow<ContainerLimitation>> ret;
            ret.discard();
            return ret;
          }
          if (output.isError())
            throw std::runtime_error("Unable to parse output: " +
                                     output.error());

          if (output->empty()) throw should_continue_exception();

          Result<ContainerLimitation> containerLimitation =
              jsonToProtobuf<ContainerLimitation>(output.get());

          if (containerLimitation.isError())
            throw std::runtime_error(
                "Unable to deserialize ContainerLimitation: " +
                containerLimitation.error());

          return Break(containerLimitation.get());
        } catch (const std::runtime_error& e) {
          if (e.what()) LOG(WARNING) << e.what();
        } catch (const should_continue_exception& e) {
        }
        return after(Seconds(command.frequence()))
            .then([]() -> process::ControlFlow<ContainerLimitation> {
              return process::Continue();
            });
      });

  future.onAny([proc]() {
    LOG(WARNING) << "Terminating watch loop";
    terminate(proc);
    LOG(WARNING) << "Watch loop Terminated";
  });

  return future;
}

process::Future<::mesos::ResourceStatistics> CommandIsolatorProcess::usage(
    const ContainerID& containerId) {
  double now = Clock::now().secs();

  if (m_usageCommand.isNone()) return emptyStats(now);

  logging::Metadata metadata = {containerId.value(), "usage"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  if (m_infos.contains(containerId)) {
    inputsJson.values["container_config"] =
        JSON::protobuf(m_infos[containerId]);
  } else {
    return Failure(
        "mesos-command-module is not initialized for current container");
  }

  return CommandRunner(m_isDebugMode, metadata)
      .asyncRun(m_usageCommand.get(), stringify(inputsJson))
      .then([now = now](Try<string> output)
                ->Future<::mesos::ResourceStatistics> {
                  if (output.isError()) {
                    LOG(WARNING) << "Unable to parse output: "
                                 << output.error();
                    return emptyStats(now);
                  }
                  if (output->empty()) {
                    LOG(WARNING) << "Output is empty";
                    return emptyStats(now);
                  }
                  Result<::mesos::ResourceStatistics> resourceStatistics =
                      jsonToProtobuf<::mesos::ResourceStatistics>(output.get());

                  if (resourceStatistics.isError()) {
                    LOG(WARNING) << "Unable to deserialize ResourceStatistics: "
                                 << resourceStatistics.error();
                    return emptyStats(now);
                  }
                  return resourceStatistics.get();
                })
      .recover([now = now](const Future<::mesos::ResourceStatistics>& result)
                   ->Future<::mesos::ResourceStatistics> {
                     LOG(WARNING) << "Failed to run usage command: " << result;
                     return emptyStats(now);
                   });
}

process::Future<Nothing> CommandIsolatorProcess::cleanup(
    const ContainerID& containerId) {
  if (m_cleanupCommand.isNone()) {
    cleanContainerContext(containerId);
    return Nothing();
  }

  logging::Metadata metadata = {containerId.value(), "cleanup"};

  JSON::Object inputsJson;
  inputsJson.values["container_id"] = JSON::protobuf(containerId);
  if (m_infos.contains(containerId)) {
    inputsJson.values["container_config"] =
        JSON::protobuf(m_infos[containerId]);
    Try<string> output =
        CommandRunner(m_isDebugMode, metadata)
            .run(m_cleanupCommand.get(), stringify(inputsJson));

    cleanContainerContext(containerId);
    if (output.isError()) {
      return Failure(output.error());
    }
  } else {
    LOG(WARNING) << "Missing container info during cleanup of "
                    "mesos-command-module, won't call command.";
  }

  return Nothing();
}

CommandIsolator::CommandIsolator(const string& name,
                                 const Option<Command>& prepareCommand,
                                 const Option<Command>& isolateCommand,
                                 const Option<RecurrentCommand>& watchCommand,
                                 const Option<Command>& cleanupCommand,
                                 const Option<Command>& usageCommand,
                                 bool isDebugMode)
    : m_process(new CommandIsolatorProcess(name, prepareCommand, isolateCommand,
                                           watchCommand, cleanupCommand,
                                           usageCommand, isDebugMode)) {
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

process::Future<Nothing> CommandIsolator::isolate(
    const ContainerID& containerId, const pid_t pid) {
  return dispatch(m_process, &CommandIsolatorProcess::isolate, containerId,
                  pid);
}

process::Future<Nothing> CommandIsolator::recover(
    const std::vector<ContainerState>& states,
    const hashset<ContainerID>& orphans) {
  return dispatch(m_process, &CommandIsolatorProcess::recover, states, orphans);
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

bool CommandIsolator::hasContainerContext(const ContainerID& containerId) {
  return m_process->hasContainerContext(containerId);
}

const Option<Command>& CommandIsolator::prepareCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->prepareCommand();
}

const Option<Command>& CommandIsolator::isolateCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->isolateCommand();
}

const Option<Command>& CommandIsolator::cleanupCommand() const {
  CHECK_NOTNULL(m_process);
  return m_process->cleanupCommand();
}
}  // namespace mesos
}  // namespace criteo
