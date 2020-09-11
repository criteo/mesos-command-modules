#include "CommandResourceEstimator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

#include <google/protobuf/text_format.h>

#include <process/defer.hpp>
#include <process/dispatch.hpp>

namespace criteo {
namespace mesos {

using namespace process;
using namespace mesos;

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
      const lambda::function<Future<ResourceUsage>()>& _usage,
      const Option<Command>& oversubscribableCommand, bool isDebugMode);

  virtual process::Future<Resources> oversubscribable();
  virtual process::Future<Resources> _oversubscribable(
      const ResourceUsage& usage);

 protected:
  const lambda::function<Future<ResourceUsage>()> usage;

 private:
  Option<Command> m_oversubscribableCommand;
  bool m_isDebugMode;
};

CommandResourceEstimatorProcess::CommandResourceEstimatorProcess(
    const lambda::function<process::Future<ResourceUsage>()>& _usage,
    const Option<Command>& oversubscribableCommand, bool isDebugMode)
    : usage(_usage),
      m_oversubscribableCommand(oversubscribableCommand),
      m_isDebugMode(isDebugMode) {}

Future<Resources> CommandResourceEstimatorProcess::_oversubscribable(
    const ResourceUsage& usage) {
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
  google::protobuf::TextFormat::PrintToString(usage, &input);

  Try<string> output = cmdrunner.run(m_oversubscribableCommand.get(), input);
  if (output.isError()) {
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
  return usage().then(defer(self(), &Self::_oversubscribable, lambda::_1));
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
    const lambda::function<Future<::mesos::ResourceUsage>()>& _usage) {
  process = new CommandResourceEstimatorProcess(_usage, m_oversubscribable,
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
