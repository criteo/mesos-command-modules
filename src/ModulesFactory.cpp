#include "ModulesFactory.hpp"

#include "CommandHook.hpp"
#include "CommandIsolator.hpp"
#include "ConfigurationParser.hpp"

namespace criteo {
namespace mesos {

using std::map;
using std::string;

::mesos::Hook* createHook(const ::mesos::Parameters& parameters) {
  Configuration cfg = ConfigurationParser::parse(parameters);
  return new CommandHook(cfg.slaveRunTaskLabelDecoratorCommand,
                         cfg.slaveExecutorEnvironmentDecoratorCommand,
                         cfg.slaveRemoveExecutorHookCommand, cfg.isDebugSet);
}

::mesos::slave::Isolator* createIsolator(
    const ::mesos::Parameters& parameters) {
  Configuration cfg = ConfigurationParser::parse(parameters);
  return new CommandIsolator(cfg.prepareCommand, cfg.watchCommand,
                             cfg.cleanupCommand, cfg.usageCommand,
                             cfg.isDebugSet);
}
}  // namespace mesos
}  // namespace criteo

mesos::modules::Module<::mesos::Hook> com_criteo_mesos_CommandHook(
    MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos", "mesos@criteo.com",
    "Command hook module", nullptr, criteo::mesos::createHook);

mesos::modules::Module<::mesos::slave::Isolator>
    com_criteo_mesos_CommandIsolator(MESOS_MODULE_API_VERSION, MESOS_VERSION,
                                     "Criteo Mesos", "mesos@criteo.com",
                                     "Command isolator module", nullptr,
                                     criteo::mesos::createIsolator);
