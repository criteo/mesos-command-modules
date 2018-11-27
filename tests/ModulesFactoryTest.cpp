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
  var->set_key("hook_slave_run_task_label_decorator_command");
  var->set_value("command_slave_run_task_label_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_remove_executor_hook_command");
  var->set_value("command_slave_remove_executor_hook");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_EQ(hook->runTaskLabelCommand().get(),
            Command("command_slave_run_task_label_decorator", 30));
  ASSERT_EQ(hook->executorEnvironmentCommand().get(),
            Command("command_slave_executor_environment_decorator", 30));
  ASSERT_EQ(hook->removeExecutorCommand().get(),
            Command("command_slave_remove_executor_hook", 30));
}

TEST(ModulesFactoryTest, should_create_hook_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_TRUE(hook->runTaskLabelCommand().isNone());
  ASSERT_EQ(hook->executorEnvironmentCommand().get(),
            Command("command_slave_executor_environment_decorator", 30));
  ASSERT_TRUE(hook->removeExecutorCommand().isNone());
}

// ***************************************
// ************** Isolator ***************
// ***************************************
TEST(ModulesFactoryTest, should_create_isolator_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("isolator_prepare_command");
  var->set_value("command_prepare");

  var = parameters.add_parameter();
  var->set_key("isolator_cleanup_command");
  var->set_value("command_cleanup");

  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand().get(), Command("command_prepare", 30));
  ASSERT_EQ(isolator->cleanupCommand().get(), Command("command_cleanup", 30));
}

TEST(ModulesFactoryTest, should_create_isolator_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("isolator_prepare_command");
  var->set_value("command_prepare");

  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand().get(), Command("command_prepare", 30));
  ASSERT_TRUE(isolator->cleanupCommand().isNone());
}
