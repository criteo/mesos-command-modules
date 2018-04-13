#include <gtest/gtest.h>
#include "ModulesFactory.hpp"
#include "CommandHook.hpp"

using namespace criteo::mesos;

TEST(ModulesFactoryTest, should_create_hook_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("slave_run_task_label_decorator");
  var->set_value("command_slave_run_task_label_decorator");

  var = parameters.add_parameter();
  var->set_key("slave_executor_environment_decorator");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("slave_remove_executor_hook");
  var->set_value("command_slave_remove_executor_hook");

  std::unique_ptr<CommandHook> hook(dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_EQ(hook->runTaskLabelCommand(), "command_slave_run_task_label_decorator");
  ASSERT_EQ(hook->executorEnvironmentCommand(), "command_slave_executor_environment_decorator");
  ASSERT_EQ(hook->removeExecutorCommand(), "command_slave_remove_executor_hook");
}

TEST(ModulesFactoryTest, should_create_hook_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("slave_executor_environment_decorator");
  var->set_value("command_slave_executor_environment_decorator");

  std::unique_ptr<CommandHook> hook(dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_TRUE(hook->runTaskLabelCommand().empty());
  ASSERT_EQ(hook->executorEnvironmentCommand(), "command_slave_executor_environment_decorator");
  ASSERT_TRUE(hook->removeExecutorCommand().empty());
}
