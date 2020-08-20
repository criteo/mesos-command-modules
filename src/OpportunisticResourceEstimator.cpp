#include "OpportunisticResourceEstimator.hpp"
#include "CommandRunner.hpp"

#include <process/defer.hpp>
#include <process/dispatch.hpp>

namespace criteo {
namespace mesos {

using namespace process;
using namespace mesos;
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
  OpportunisticResourceEstimatorProcess();

  Future<Resources> oversubscribable() {
    return (usage().then(
        defer(self(), &Self::_oversubscribable)));  //, lambda::_1));
  }

  // look for underscore meaning private ?
  Future<Resources> _oversubscribable() {
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
  const lambda::function<Future<ResourceUsage>()> usage;
  const ::mesos::Resources totalRevocable;
};

// resource estimator class is define in .hpp
OpportunisticResourceEstimator::OpportunisticResourceEstimator(
    const Resources& _totalRevocable) {
  // Mark all resources as revocable.
  foreach (Resource resource, _totalRevocable) {
    resource.mutable_revocable();
    totalRevocable += resource;
  }
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
  return Nothing();
}

Future<Resources> OpportunisticResourceEstimator::oversubscribable() {
  if (process == nullptr) {
    return Failure("Opportunistic resource estimator is not initialized");
  }

  return dispatch(process,
                  &OpportunisticResourceEstimatorProcess::oversubscribable);
}

}  // namespace mesos
}  // namespace criteo
