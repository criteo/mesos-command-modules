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

  Future<Resources> oversubscribable() {
    return (usage().then(
        defer(self(), &Self::_oversubscribable)));  //, lambda::_1));
  }

  // look for underscore meaning private ?
  Future<Resources> _oversubscribable() {
    // this is the function that should do all the real work
    // Command hook should take place here
    Resources allocatedRevocable;
    /*foreach (const ResourceUsage::Executor& executor, usage.executors()) {
      allocatedRevocable += Resources(executor.allocated()).revocable();
    }*/
    auto unallocated = [](const Resources& resources) {
      Resources result = resources;
      result.unallocate();
      return result;
    };

    return totalRevocable - unallocated(allocatedRevocable);
  }

 protected:
  // const string m_name;
  const lambda::function<Future<ResourceUsage>()> usage;
  const ::mesos::Resources totalRevocable;

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
  /*Try<Resources> _resources = ::mesos::Resources::parse(
      "[{\"name\" : \"cpus\", \"type\" : \"SCALAR\", \"scalar\" : {\"value\" : "
      "8}}]");
  if (_resources.isError()) {
    totalRevocable = nullptr;
  }
  totalRevocable = _resources.get().get();*/
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
