#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include <stout/option.hpp>
#include <mesos/mesos.pb.h>

#include "Command.hpp"

namespace criteo {
namespace mesos {

struct Configuration {
  Option<Command> prepareCommand;
  Option<Command> cleanupCommand;

  Option<Command> slaveRunTaskLabelDecoratorCommand;
  Option<Command> slaveExecutorEnvironmentDecoratorCommand;
  Option<Command> slaveRemoveExecutorHookCommand;

  bool isDebugSet;
};

class ConfigurationParser
{
public:
  static Configuration parse(const ::mesos::Parameters& parameters);
};
}
}

#endif // CONFIGURATION_PARSER_HPP
