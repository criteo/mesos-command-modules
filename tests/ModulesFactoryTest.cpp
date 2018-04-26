#include "ModulesFactory.hpp"
#include <gtest/gtest.h>
#include "CommandHook.hpp"
#include "CommandIsolator.hpp"

using namespace criteo::mesos;

// ***************************************
// **************** Hook *****************
// ***************************************
TEST(ModulesFactoryTest, should_create_hook_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_run_task_label_decorator");
  var->set_value("command_slave_run_task_label_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_remove_executor_hook");
  var->set_value("command_slave_remove_executor_hook");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_EQ(hook->runTaskLabelCommand(),
            "command_slave_run_task_label_decorator");
  ASSERT_EQ(hook->executorEnvironmentCommand(),
            "command_slave_executor_environment_decorator");
  ASSERT_EQ(hook->removeExecutorCommand(),
            "command_slave_remove_executor_hook");
}

TEST(ModulesFactoryTest, should_create_hook_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator");
  var->set_value("command_slave_executor_environment_decorator");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_TRUE(hook->runTaskLabelCommand().empty());
  ASSERT_EQ(hook->executorEnvironmentCommand(),
            "command_slave_executor_environment_decorator");
  ASSERT_TRUE(hook->removeExecutorCommand().empty());
}

// ***************************************
// ************** Isolator ***************
// ***************************************
TEST(ModulesFactoryTest, should_create_isolator_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("isolator_prepare");
  var->set_value("command_prepare");

  var = parameters.add_parameter();
  var->set_key("isolator_cleanup");
  var->set_value("command_cleanup");

  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand(), "command_prepare");
  ASSERT_EQ(isolator->cleanupCommand(), "command_cleanup");
}

TEST(ModulesFactoryTest, should_create_isolator_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("isolator_prepare");
  var->set_value("command_prepare");

  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand(), "command_prepare");
  ASSERT_TRUE(isolator->cleanupCommand().empty());
}
