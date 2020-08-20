#include <mesos/module/resource_estimator.hpp>

#include <mesos/slave/resource_estimator.hpp>

namespace criteo{
namespace mesos{

class OpportunisticResourceEstimatorProcess
  : public Process<OpportunisticResourceEstimatorProcess>
{
public:
  OpportunisticResourceEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& _usage,
      const Resources& _totalRevocable)
    : ProcessBase(process::ID::generate("Opportunistic-resource-estimator")),
      usage(_usage),
      totalRevocable(_totalRevocable) {}

  Future<Resources> oversubscribable()
  {
    return usage().then(defer(self(), &Self::_oversubscribable, lambda::_1));
  }

  Future<Resources> _oversubscribable(const ResourceUsage& usage)
  {
    Resources allocatedRevocable;
    foreach (const ResourceUsage::Executor& executor, usage.executors()) {
      allocatedRevocable += Resources(executor.allocated()).revocable();
    }

    auto unallocated = [](const Resources& resources) {
      Resources result = resources;
      result.unallocate();
      return result;
    };

    return totalRevocable - unallocated(allocatedRevocable);
  }

protected:
  const lambda::function<Future<ResourceUsage>()> usage;
  const Resources TotalRevocable;
};



class OpportunisticResourceEstimator : public ResourceEstimator
{
public:
  OpportunisticResourceEstimator(const Resources& _totalRevocable)
  {
    // Mark all resources as revocable.
    foreach (Resource resource, _totalRevocable) {
      resource.mutable_revocable();
      totalRevocable += resource;
    }
  }

   ~OpportunisticResourceEstimator() override
  {
    if (process.get() != nullptr) {
      terminate(process.get());
      wait(process.get());
    }
  }

Try<Nothing> initialize(
      const lambda::function<Future<ResourceUsage>()>& usage) override
  {
    if (process.get() != nullptr) {
      return Error("Opportunistic resource estimator has already been initialized");
    }

    process.reset(new OpportunisticResourceEstimatorProcess(usage, totalRevocable));
    spawn(process.get());

    return Nothing();
  }

  Future<Resources> oversubscribable() override
  {
    if (process.get() == nullptr) {
      return Failure("Opportunistic resource estimator is not initialized");
    }

    return dispatch(
        process.get(),
        &OpportunisticResourceEstimatorProcess::oversubscribable);
  }

private:
  Resources totalRevocable;
  Owned<OpportunisticResourceEstimatorProcess> process;
};


static bool compatible()
{
  return true;
}


static ResourceEstimator* create(const Parameters& parameters)
{
  // Obtain the *fixed* resources from parameters.
  Option<Resources> resources;
  foreach (const Parameter& parameter, parameters.parameter()) {
    if (parameter.key() == "resources") {
      Try<Resources> _resources = Resources::parse(parameter.value());
      if (_resources.isError()) {
        return nullptr;
      }

      resources = _resources.get();
    }
  }

  if (resources.isNone()) {
    return nullptr;
  }

  return new OpportunisticResourceEstimator(resources.get());
}


Module<ResourceEstimator> org_apache_mesos_OpportunisticResourceEstimator(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Apache Mesos",
    "modules@mesos.apache.org",
    "Fixed Resource Estimator Module.",
    compatible,
    create);
}//namespace mesos
}//namespace criteo
