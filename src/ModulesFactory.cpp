#include "ModulesFactory.hpp"
#include "CommandHook.hpp"
#include "CommandIsolator.hpp"

namespace criteo {
namespace mesos {

using std::map;
using std::string;

/*
 * Parameters for hook
 */
const string SLAVE_RUN_TASK_LABEL_DECORATOR_KEY =
    "hook_slave_run_task_label_decorator";
const string SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY =
    "hook_slave_executor_environment_decorator";
const string SLAVE_REMOVE_EXECUTOR_KEY = "hook_slave_remove_executor_hook";

/*
 * Parameters for isolator
 */
const string PREPARE_KEY = "isolator_prepare";
const string CLEANUP_KEY = "isolator_cleanup";

const string DEBUG_KEY = "debug";

map<string, string> toMap(const ::mesos::Parameters& parameters) {
  map<string, string> kv;
  foreach (const ::mesos::Parameter& parameter, parameters.parameter()) {
    if (parameter.has_key() && parameter.has_value()) {
      kv[parameter.key()] = parameter.value();
    }
  }
  return kv;
}

string getOrEmpty(const map<string, string>& kv, const string& key) {
  string command;
  auto it = kv.find(key);
  if (it != kv.end()) command = it->second;
  return command;
}

::mesos::Hook* createHook(const ::mesos::Parameters& parameters) {
  map<string, string> p = toMap(parameters);
  string runTaskLabelCommand =
      getOrEmpty(p, SLAVE_RUN_TASK_LABEL_DECORATOR_KEY);
  string executorEnvironmentCommand =
      getOrEmpty(p, SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY);
  string removeExecutorCommand = getOrEmpty(p, SLAVE_REMOVE_EXECUTOR_KEY);
  bool isDebugMode = getOrEmpty(p, DEBUG_KEY) == "true";
  return new CommandHook(runTaskLabelCommand, executorEnvironmentCommand,
                         removeExecutorCommand, isDebugMode);
}

::mesos::slave::Isolator* createIsolator(
    const ::mesos::Parameters& parameters) {
  map<string, string> p = toMap(parameters);
  string prepareCommand = getOrEmpty(p, PREPARE_KEY);
  string cleanupCommand = getOrEmpty(p, CLEANUP_KEY);
  bool isDebugMode = getOrEmpty(p, DEBUG_KEY) == "true";
  return new CommandIsolator(prepareCommand, cleanupCommand, isDebugMode);
}
}
}

mesos::modules::Module<::mesos::Hook> com_criteo_mesos_CommandHook(
    MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos", "mesos@criteo.com",
    "Command hook module", nullptr, criteo::mesos::createHook);

mesos::modules::Module<::mesos::slave::Isolator>
    com_criteo_mesos_CommandIsolator(MESOS_MODULE_API_VERSION, MESOS_VERSION,
                                     "Criteo Mesos", "mesos@criteo.com",
                                     "Command isolator module", nullptr,
                                     criteo::mesos::createIsolator);
