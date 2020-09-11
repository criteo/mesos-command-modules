#include "CommandResourceEstimator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

#include <google/protobuf/text_format.h>

#include <process/defer.hpp>
#include <process/dispatch.hpp>

namespace criteo {
namespace mesos {

using std::string;

using ::mesos::modules::Module;
using ::mesos::Resource;
using ::mesos::Resources;
using ::mesos::slave::ResourceEstimator;
using ::mesos::ResourceUsage;

using process::Future;
using process::Failure;
using process::terminate;

class CommandResourceEstimatorProcess
    : public process::Process<CommandResourceEstimatorProcess> {
 public:
  CommandResourceEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& usage,
      const Option<Command>& oversubscribableCommand, bool isDebugMode);

  virtual process::Future<Resources> oversubscribable();
  virtual Resources m_oversubscribable(const ResourceUsage& usage);

 protected:
  const lambda::function<Future<ResourceUsage>()> m_usage;

 private:
  Option<Command> m_oversubscribableCommand;
  bool m_isDebugMode;
};

CommandResourceEstimatorProcess::CommandResourceEstimatorProcess(
    const lambda::function<process::Future<ResourceUsage>()>& usage,
    const Option<Command>& oversubscribableCommand, bool isDebugMode)
    : m_usage(usage),
      m_oversubscribableCommand(oversubscribableCommand),
      m_isDebugMode(isDebugMode) {}

Resources CommandResourceEstimatorProcess::m_oversubscribable(
    const ResourceUsage& m_usage) {
  // Mocking a resources to have valid return
  Try<Resources> _resources = ::mesos::Resources::parse(
      "[{\"name\" : \"cpus\", \"type\":\"SCALAR\", \"scalar\" : {\"value\" : "
      "\"0\"}, \"role\" : \"*\"}]");
  Resources resources = _resources.get();

  if (m_oversubscribableCommand.isNone()) {
    return resources;
  }
  logging::Metadata metadata = {"0", "oversubscribable"};

  CommandRunner cmdrunner = CommandRunner(m_isDebugMode, metadata);

  string input;
  google::protobuf::TextFormat::PrintToString(m_usage, &input);

  Try<string> output = cmdrunner.run(m_oversubscribableCommand.get(), input);
  if (output.isError()) {
    LOG(INFO) << output.error();
    // return Failure("output from CommandRunner gave error: " +
    // output.error());
    return resources;
  }

  Try<Resources> _cmdresources = ::mesos::Resources::parse(output.get());

  Resources cmdresources = _cmdresources.get();
  Resources revocable{};
  foreach (Resource resource, cmdresources) {
    resource.mutable_revocable();
    revocable += resource;
  }

  return revocable;
}
Future<Resources> CommandResourceEstimatorProcess::oversubscribable() {
  return m_usage().then(defer(self(), &Self::m_oversubscribable, lambda::_1));
}

CommandResourceEstimator::CommandResourceEstimator(
    const Option<Command>& _oversubscribable, bool isDebugMode)
    : m_oversubscribable(_oversubscribable), m_isDebugMode(isDebugMode) {}

CommandResourceEstimator::~CommandResourceEstimator() {
  if (process != nullptr) {
    terminate(process);
    wait(process);
  }
}

Try<Nothing> CommandResourceEstimator::initialize(
    const lambda::function<Future<::mesos::ResourceUsage>()>& usage) {
  process = new CommandResourceEstimatorProcess(usage, m_oversubscribable,
                                                m_isDebugMode);
  spawn(process);

  return Nothing();
}

Future<Resources> CommandResourceEstimator::oversubscribable() {
  if (process == nullptr) {
    return Failure("Opportunistic resource estimator is not initialized");
  }

  return dispatch(process, &CommandResourceEstimatorProcess::oversubscribable);
}

}  // namespace mesos
}  // namespace criteo
