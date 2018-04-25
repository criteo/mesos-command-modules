#ifndef __COMMAND_RUNNER_HPP__
#define __COMMAND_RUNNER_HPP__

#include <string>

#include <stout/try.hpp>

namespace criteo {
namespace mesos {
namespace CommandRunner
{
Try<std::string> run(const std::string& command, const std::string& serializedInput,
  int timeout = 5);
}
}
}

#endif
