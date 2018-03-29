#ifndef __MODULES_FACTORY_HPP__
#define __MODULES_FACTORY_HPP__

#include <mesos/hook.hpp>
#include <mesos/module/hook.hpp>

namespace criteo {
namespace mesos {
::mesos::Hook* createHook(const ::mesos::Parameters& parameters);
}
}

#endif
