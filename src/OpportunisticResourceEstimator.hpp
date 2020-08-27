#ifndef __OPPORTUNISTIC_RESOURCE_ESTIMATOR_HPP__
#define __OPPORTUNISTIC_RESOURCE_ESTIMATOR_HPP__

#include <string>

#include <mesos/module/resource_estimator.hpp>
#include <mesos/resources.hpp>
#include <process/process.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>

#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>

#include "CommandRunner.hpp"

namespace criteo {
namespace mesos {

class OpportunisticResourceEstimatorProcess;

class OpportunisticResourceEstimator
    : public ::mesos::slave::ResourceEstimator {
 public:
  explicit OpportunisticResourceEstimator(
      const Option<Command>& oversubscribable, bool isDebugMode);
  // const ::mesos::Resources& _totalRevocable);

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<::mesos::ResourceUsage>()>& usage);
  virtual ~OpportunisticResourceEstimator();

  virtual process::Future<::mesos::Resources> oversubscribable();

 private:
  ::mesos::Resources totalRevocable;
  OpportunisticResourceEstimatorProcess* process;
  bool m_isDebugMode;
};
}  // namespace mesos
}  // namespace criteo
#endif
