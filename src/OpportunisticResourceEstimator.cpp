#include "OpportunisticResourceEstimator.hpp"
#include "CommandRunner.hpp"

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

  virtual process::Future<Resources> oversubscribable(); /* {
     // do the thing here
     if (m_oversubscribableCommand.isNone()) {
       return None();
     }
     JSON::Object inputsJson;
     logging::Metadata metadata = {0, "overscubribable"};

     Try<string> output =
         CommandRunner(m_isDebugMode, metadata)
             .run(m_oversuscribable.get(), stringify(inputsJson));
     if (ouput.isError()) {
       return Failure(output.error());
     }

     return totalRevocable;
   }*/

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
      "8}}]");
  if (!_resources.isError()) {
    totalRevocable = _resources.get();
  }
}

Future<Resources> OpportunisticResourceEstimatorProcess::oversubscribable() {
  // do the thing here
  if (m_oversubscribableCommand.isNone()) {
    Try<Resources> _resources = ::mesos::Resources::parse("[{}]");
    return _resources;
  }
  JSON::Object inputsJson;
  logging::Metadata metadata = {0, "overscubribable"};

  Try<string> output =
      CommandRunner(m_isDebugMode, metadata)
          .run(m_oversubscribableCommand.get(), stringify(inputsJson));
  if (output.isError()) {
    return Failure(output.error());
  }
  return totalRevocable;
}

// resource estimator class is define in .hpp
OpportunisticResourceEstimator::OpportunisticResourceEstimator(
    const Option<Command>& oversubscribable, bool isDebugMode)
    : process(new OpportunisticResourceEstimatorProcess(oversubscribable,
                                                        isDebugMode)) {
  spawn(process);

  LOG(INFO) << "!!!!!!!!!!!! RESOURCE ESTIMATOR IN ACTION !!!!!!!!!!!";
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
