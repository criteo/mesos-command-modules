#include "ConfigurationParser.hpp"

#include <map>
#include <stout/foreach.hpp>

namespace criteo {
namespace mesos {

using std::map;
using std::stoul;
using std::string;

// Hook commands.
const string SLAVE_RUN_TASK_LABEL_DECORATOR_KEY =
    "hook_slave_run_task_label_decorator";
const string SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY =
    "hook_slave_executor_environment_decorator";
const string SLAVE_REMOVE_EXECUTOR_KEY = "hook_slave_remove_executor_hook";

// Isolator commands.
const string PREPARE_KEY = "isolator_prepare";
const string WATCH_KEY = "isolator_watch";
const string CLEANUP_KEY = "isolator_cleanup";
const string USAGE_KEY = "isolator_usage";

// Additional parameters.
const string DEBUG_KEY = "debug";  // enable debug mode.

const string MODULE_NAME_KEY = "module_name";

string getOrEmpty(const map<string, string>& kv, const string& key) {
  string command;
  auto it = kv.find(key);
  if (it != kv.end()) command = it->second;
  return command;
}

map<string, string> toMap(const ::mesos::Parameters& parameters) {
  map<string, string> kv;
  foreach (const ::mesos::Parameter& parameter, parameters.parameter()) {
    if (parameter.has_key() && parameter.has_value()) {
      kv[parameter.key()] = parameter.value();
    }
  }
  return kv;
}

Option<Command> extractCommand(const map<string, string>& kv,
                               const std::string& commandKey) {
  string cmd = getOrEmpty(kv, commandKey + "_command");
  if (!cmd.empty()) {
    Command command = Command(cmd);

    string timeoutStr = getOrEmpty(kv, commandKey + "_timeout");
    if (!timeoutStr.empty()) {
      unsigned long timeout = stoul(timeoutStr);
      command.setTimeout(timeout);
    }

    return Option<Command>(command);
  }
  return Option<Command>();
}

Option<RecurrentCommand> extractRecurrentCommand(
    const map<string, string>& kv, const std::string& commandKey) {
  Option<Command> baseCmd = extractCommand(kv, commandKey);

  if (baseCmd.isNone()) return Option<RecurrentCommand>();

  RecurrentCommand command = RecurrentCommand(baseCmd.get());

  string frequenceStr = getOrEmpty(kv, commandKey + "_frequence");
  if (!frequenceStr.empty()) {
    unsigned long frequence = stoul(frequenceStr);
    command.setFrequence(frequence);
  }

  return Option<RecurrentCommand>(command);
}

Configuration ConfigurationParser::parse(
    const ::mesos::Parameters& parameters) {
  map<string, string> p = toMap(parameters);
  Configuration configuration;

  configuration.slaveExecutorEnvironmentDecoratorCommand =
      extractCommand(p, SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY);
  configuration.slaveRemoveExecutorHookCommand =
      extractCommand(p, SLAVE_REMOVE_EXECUTOR_KEY);
  configuration.slaveRunTaskLabelDecoratorCommand =
      extractCommand(p, SLAVE_RUN_TASK_LABEL_DECORATOR_KEY);

  configuration.prepareCommand = extractCommand(p, PREPARE_KEY);
  configuration.watchCommand = extractRecurrentCommand(p, WATCH_KEY);
  configuration.cleanupCommand = extractCommand(p, CLEANUP_KEY);
  configuration.usageCommand = extractCommand(p, USAGE_KEY);

  configuration.isDebugSet = getOrEmpty(p, DEBUG_KEY) == "true";

  configuration.name = getOrEmpty(p, MODULE_NAME_KEY);
  if (configuration.name.empty())
    throw std::runtime_error(MODULE_NAME_KEY +
                             " parameter is mandatory for all defined modules");

  return configuration;
}

}  // namespace mesos
}  // namespace criteo
