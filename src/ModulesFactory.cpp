#include "ModulesFactory.hpp"

#include "CommandHook.hpp"
#include "CommandIsolator.hpp"
#include "CommandQoSController.hpp"
#include "CommandResourceEstimator.hpp"
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
  return new CommandIsolator(cfg.name, cfg.prepareCommand, cfg.isolateCommand,
                             cfg.watchCommand, cfg.cleanupCommand,
                             cfg.usageCommand, cfg.isDebugSet);
}

::mesos::slave::ResourceEstimator* createResourceEstimator(
    const ::mesos::Parameters& parameters) {
  Configuration cfg = ConfigurationParser::parse(parameters);
  return new CommandResourceEstimator(cfg.name, cfg.oversubscribableCommand,
                                      cfg.isDebugSet);
}

::mesos::slave::QoSController* createQoSController(
    const ::mesos::Parameters& parameters) {
  Configuration cfg = ConfigurationParser::parse(parameters);
  return new CommandQoSController(cfg.name, cfg.correctionsCommand,
                                  cfg.isDebugSet);
}

}  // namespace mesos
}  // namespace criteo

mesos::modules::Module<::mesos::Hook> com_criteo_mesos_CommandHook(
    MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos", "mesos@criteo.com",
    "Command hook module", nullptr, criteo::mesos::createHook);
// Mesos does not accept to instanciate several instances of the same module
// so we create multiple instances named com_criteo_mesos_CommandHook2,3,..
// to allow user to have separated isolator based on CommandHook
#define CRITEO_HOOK(INSTANCE)                                      \
  mesos::modules::Module<::mesos::Hook>                            \
      com_criteo_mesos_CommandHook##INSTANCE(                      \
          MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos", \
          "mesos@criteo.com", "Command hook module", nullptr,      \
          criteo::mesos::createHook);

CRITEO_HOOK(2)
CRITEO_HOOK(3)
CRITEO_HOOK(4)
CRITEO_HOOK(5)
CRITEO_HOOK(6)
CRITEO_HOOK(7)
CRITEO_HOOK(8)
CRITEO_HOOK(9)
CRITEO_HOOK(10)
CRITEO_HOOK(11)

mesos::modules::Module<::mesos::slave::Isolator>
    com_criteo_mesos_CommandIsolator(MESOS_MODULE_API_VERSION, MESOS_VERSION,
                                     "Criteo Mesos", "mesos@criteo.com",
                                     "Command isolator module", nullptr,
                                     criteo::mesos::createIsolator);
// Mesos does not accept to instanciate several instances of the same module
// so we create multiple instances named com_criteo_mesos_CommandIsolator2,3,..
// to allow user to have separated isolator based on CommandIsolator
#define CRITEO_ISOLATOR(INSTANCE)                                  \
  mesos::modules::Module<::mesos::slave::Isolator>                 \
      com_criteo_mesos_CommandIsolator##INSTANCE(                  \
          MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos", \
          "mesos@criteo.com", "Command isolator module", nullptr,  \
          criteo::mesos::createIsolator);

CRITEO_ISOLATOR(2)
CRITEO_ISOLATOR(3)
CRITEO_ISOLATOR(4)
CRITEO_ISOLATOR(5)
CRITEO_ISOLATOR(6)
CRITEO_ISOLATOR(7)
CRITEO_ISOLATOR(8)
CRITEO_ISOLATOR(9)
CRITEO_ISOLATOR(10)
CRITEO_ISOLATOR(11)

mesos::modules::Module<::mesos::slave::ResourceEstimator>
    com_criteo_mesos_CommandResourceEstimator(
        MESOS_MODULE_API_VERSION, MESOS_VERSION, "Criteo Mesos",
        "mesos@criteo.com", "Command resource estimator module", nullptr,
        criteo::mesos::createResourceEstimator);

mesos::modules::Module<::mesos::slave::QoSController>
    com_criteo_mesos_CommandQoSController(MESOS_MODULE_API_VERSION,
                                          MESOS_VERSION, "Criteo Mesos",
                                          "mesos@criteo.com",
                                          "Command QoSController module",
                                          nullptr,
                                          criteo::mesos::createQoSController);
