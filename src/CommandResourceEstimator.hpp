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

class CommandResourceEstimatorProcess;

class CommandResourceEstimator : public ::mesos::slave::ResourceEstimator {
 public:
  explicit CommandResourceEstimator(const Option<Command>& oversubscribable,
                                    bool isDebugMode);

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<::mesos::ResourceUsage>()>& usage);
  virtual ~CommandResourceEstimator();

  virtual process::Future<::mesos::Resources> oversubscribable();

 private:
  const Option<Command> m_oversubscribable;
  CommandResourceEstimatorProcess* process;
  bool m_isDebugMode;
};
}  // namespace mesos
}  // namespace criteo
#endif
