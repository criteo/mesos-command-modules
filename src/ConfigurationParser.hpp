#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include <mesos/mesos.pb.h>
#include <stout/option.hpp>

#include "Command.hpp"

namespace criteo {
namespace mesos {

/**
 * @brief The Configuration struct contains all elements the user
 * can provide in the configuration.
 */
struct Configuration {
  Option<Command> prepareCommand;
  Option<Command> watchCommand;
  Option<Command> cleanupCommand;

  Option<Command> slaveRunTaskLabelDecoratorCommand;
  Option<Command> slaveExecutorEnvironmentDecoratorCommand;
  Option<Command> slaveRemoveExecutorHookCommand;

  // this flag allows the user to enable debug mode.
  bool isDebugSet;
};

/**
 * @brief The ConfigurationParser class parses the configuration KV provided by
 * Mesos and produces a configuration object.
 */
class ConfigurationParser {
 public:
  /**
   * @brief parse the configuration provided by the user.
   * @param parameters The parameters provided by Mesos.
   * @return The configuration for each method.
   */
  static Configuration parse(const ::mesos::Parameters& parameters);
};
}  // namespace mesos
}  // namespace criteo

#endif  // CONFIGURATION_PARSER_HPP
