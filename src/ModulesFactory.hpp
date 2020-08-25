#ifndef __MODULES_FACTORY_HPP__
#define __MODULES_FACTORY_HPP__

#include <mesos/hook.hpp>
#include <mesos/module/hook.hpp>

#include <mesos/module/isolator.hpp>
#include <mesos/slave/isolator.hpp>

#include <mesos/slave/resource_estimator.hpp>

namespace criteo {
namespace mesos {
::mesos::Hook* createHook(const ::mesos::Parameters& parameters);
::mesos::slave::Isolator* createIsolator(const ::mesos::Parameters& parameters);
::mesos::slave::ResourceEstimator* createResourceEstimator(
    const ::mesos::Parameters& parameters);

}  // namespace mesos
}  // namespace criteo

#endif
