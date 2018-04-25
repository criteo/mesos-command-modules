#include "ModulesFactory.hpp"
#include "CommandHook.hpp"

const std::string SLAVE_RUN_TASK_LABEL_DECORATOR_KEY =
  "slave_run_task_label_decorator";
const std::string SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY =
  "slave_executor_environment_decorator";
const std::string SLAVE_REMOVE_EXECUTOR_KEY =
  "slave_remove_executor_hook";


namespace criteo {
namespace mesos {

using namespace std;

map<string, string> toMap(
  const ::mesos::Parameters& parameters) {
  map<string, string> kv;
  foreach (const ::mesos::Parameter& parameter, parameters.parameter()) {
    if(parameter.has_key() && parameter.has_value()) {
      kv[parameter.key()] = parameter.value();
    }
  }
  return kv;
}

string get(const map<string, string>& kv, const string& key) {
  string command;
  auto it = kv.find(key);
  if(it != kv.end())
    command = it->second;
  return command;
}

::mesos::Hook* createHook(const ::mesos::Parameters& parameters) {
  map<string, string> p = toMap(parameters);
  auto runTaskLabelCommand = get(p, SLAVE_RUN_TASK_LABEL_DECORATOR_KEY);
  auto executorEnvironmentCommand = get(p,
    SLAVE_EXECUTOR_ENVIRONMENT_DECORATOR_KEY);
  auto removeExecutorCommand = get(p, SLAVE_REMOVE_EXECUTOR_KEY);
  return new CommandHook(runTaskLabelCommand, executorEnvironmentCommand,
    removeExecutorCommand);
}

::mesos::slave::Isolator* createIsolator(
  const ::mesos::Parameters& parameters) {
  return nullptr;
}

}
}

mesos::modules::Module<::mesos::Hook> com_criteo_mesos_CommandHook(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Criteo Mesos",
    "mesos@criteo.com",
    "Command hook module",
    nullptr,
    criteo::mesos::createHook);

mesos::modules::Module<::mesos::slave::Isolator> com_criteo_mesos_CommandIsolator(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Criteo Mesos",
    "mesos@criteo.com",
    "Command isolator module",
    nullptr,
    criteo::mesos::createIsolator);
