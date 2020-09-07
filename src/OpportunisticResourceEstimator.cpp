#include "OpportunisticResourceEstimator.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

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

// process for estimator
class OpportunisticResourceEstimatorProcess
    : public process::Process<OpportunisticResourceEstimatorProcess> {
 public:
  OpportunisticResourceEstimatorProcess(
      const Option<Command>& oversubscribableCommand, bool isDebugMode);

  virtual process::Future<Resources> oversubscribable();

 protected:
  // const string m_name;
  const lambda::function<Future<ResourceUsage>()> usage;
  ::mesos::Resources totalRevocable;

 private:
  Option<Command> m_oversubscribableCommand;
  bool m_isDebugMode;
};

// forbid declaration on global scope ?
OpportunisticResourceEstimatorProcess::OpportunisticResourceEstimatorProcess(
    const Option<Command>& oversubscribableCommand, bool isDebugMode)
    : m_oversubscribableCommand(oversubscribableCommand),
      m_isDebugMode(isDebugMode) {
  // Mock resources for totalrevocable
  Try<Resources> _resources = ::mesos::Resources::parse(
      "[{\"name\" : \"cpus\", \"type\" : \"SCALAR\", \"scalar\" : {\"value\" : "
      "\"16\"}, \"role\" : \"*\", \"revocable\" : {\"name\" : \"cpus\", "
      "\"type\" : \"SCALAR\", \"scalar\" : {\"value\" : \"8\"}}}]");
  if (!_resources.isError()) {
    totalRevocable = _resources.get();
  }
}

Future<Resources> OpportunisticResourceEstimatorProcess::oversubscribable() {
  // here the resource estimator doing
  // Mocking a resources to have valid return
  Try<Resources> _resources = ::mesos::Resources::parse(
      "[{\"name\" : \"cpus\", \"type\":\"SCALAR\", \"scalar\" : {\"value\" : "
      "\"4\"}, \"role\" : \"*\"}]");
  Resources resources = _resources.get();

  if (m_oversubscribableCommand.isNone()) {
    return resources;
  }
  LOG(INFO) << "!!!!!!!!COMMAND FOUND toto!!!!!!!!";
  logging::Metadata metadata = {"0", "oversubscribable"};

  CommandRunner cmdrunner = CommandRunner(m_isDebugMode, metadata);

  LOG(INFO) << "!!! Cmdrunner init";
  string input =
      "[{\"name\" : \"cpus\", \"type\" : \"SCALAR\", \"scalar\" : {\"value\" : "
      "\"16\"}, \"role\" : \"*\", \"revocable\" : {\"name\" : \"cpus\", "
      "\"type\" : \"SCALAR\", \"scalar\" : {\"value\" : \"8\"}}}]";

  Try<string> output = cmdrunner.run(m_oversubscribableCommand.get(), input);
  LOG(INFO) << "!!!PASSED THE PARSER!!!!!!!!!!";

  Try<Resources> _cmdresources = ::mesos::Resources::parse(output.get());
  Resources cmdresources = _cmdresources.get();
  Resources revocable{};
  foreach (Resource resource, cmdresources) {
    resource.mutable_revocable();
    revocable += resource;
    LOG(INFO) << "ouput feeback: " << resource;
  }
  // LOG(INFO) << "feeback: " << totalRevocable - cmdresources;
  if (output.isError()) {
    return resources;
    // return Error(output.error());
  }
  // returning Resources type inconsistantly crash the agent
  // need to investigate
  return revocable;
}

// resource estimator class is define in .hpp
OpportunisticResourceEstimator::OpportunisticResourceEstimator(
    const Option<Command>& oversubscribable, bool isDebugMode)
    : process(new OpportunisticResourceEstimatorProcess(oversubscribable,
                                                        isDebugMode)) {
  spawn(process);

  LOG(INFO) << "!!!!!!!!!!!!RESOURCE ESTIMATOR IN ACTION !!!!!!!!!!!";
}

// tild mean it's a destructor
OpportunisticResourceEstimator::~OpportunisticResourceEstimator() {
  if (process != nullptr) {
    terminate(process);
    wait(process);
  }
}

Try<Nothing> OpportunisticResourceEstimator::initialize(
    const lambda::function<Future<::mesos::ResourceUsage>()>& usage) {
  LOG(INFO) << "Opportunistic Resource Estimator Initialized";
  return Nothing();
}

Future<Resources> OpportunisticResourceEstimator::oversubscribable() {
  // this should be the repeat function that send the resources
  // by callin gthe process
  if (process == nullptr) {
    return Failure("Opportunistic resource estimator is not initialized");
  }
  // call oversubscribable in process
  return dispatch(process,
                  &OpportunisticResourceEstimatorProcess::oversubscribable);
}

}  // namespace mesos
}  // namespace criteo
