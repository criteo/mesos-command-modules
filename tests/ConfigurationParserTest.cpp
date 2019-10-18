#include "ConfigurationParser.hpp"

#include <gtest/gtest.h>

using namespace criteo::mesos;

TEST(ConfigurationParserTest, should_create_configuration) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_run_task_label_decorator_command");
  var->set_value("command_slave_run_task_label_decorator");
  var = parameters.add_parameter();
  var->set_key("hook_slave_run_task_label_decorator_timeout");
  var->set_value("10");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_remove_executor_hook_command");
  var->set_value("command_slave_remove_executor_hook");

  var = parameters.add_parameter();
  var->set_key("isolator_prepare_command");
  var->set_value("command_prepare");
  var = parameters.add_parameter();
  var->set_key("isolator_prepare_timeout");
  var->set_value("20");

  var = parameters.add_parameter();
  var->set_key("isolator_cleanup_command");
  var->set_value("command_cleanup");

  var = parameters.add_parameter();
  var->set_key("isolator_watch_command");
  var->set_value("command_watch");
  var = parameters.add_parameter();
  var->set_key("isolator_watch_frequence");
  var->set_value("10");

  var = parameters.add_parameter();
  var->set_key("debug");
  var->set_value("true");

  Configuration cfg = ConfigurationParser::parse(parameters);

  EXPECT_EQ(cfg.slaveRunTaskLabelDecoratorCommand, Command("command_slave_run_task_label_decorator", 10));
  EXPECT_EQ(cfg.slaveExecutorEnvironmentDecoratorCommand, Command("command_slave_executor_environment_decorator", 30));
  EXPECT_EQ(cfg.slaveRemoveExecutorHookCommand, Command("command_slave_remove_executor_hook", 30));

  EXPECT_EQ(cfg.prepareCommand, Command("command_prepare", 20));
  EXPECT_EQ(cfg.cleanupCommand, Command("command_cleanup", 30));
  EXPECT_EQ(cfg.watchCommand, Command("command_watch", 30, 10));

  EXPECT_TRUE(cfg.isDebugSet);
}

TEST(ConfigurationParserTest, should_create_configuration_with_optional_params) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();

  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("debug");
  var->set_value("true");

  Configuration cfg = ConfigurationParser::parse(parameters);

  EXPECT_TRUE(cfg.slaveRunTaskLabelDecoratorCommand.isNone());
  EXPECT_EQ(cfg.slaveExecutorEnvironmentDecoratorCommand, Command("command_slave_executor_environment_decorator", 30, 30));
  EXPECT_TRUE(cfg.slaveRemoveExecutorHookCommand.isNone());

  EXPECT_TRUE(cfg.prepareCommand.isNone());
  EXPECT_TRUE(cfg.cleanupCommand.isNone());

  EXPECT_TRUE(cfg.isDebugSet);
}

TEST(ConfigurationParserTest, should_set_debug_only_with_true_value) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("debug");
  var->set_value("true");

  Configuration cfg = ConfigurationParser::parse(parameters);
  EXPECT_TRUE(cfg.isDebugSet);

  var->set_value("false");
  cfg = ConfigurationParser::parse(parameters);
  EXPECT_FALSE(cfg.isDebugSet);

  var->set_value("anything123");
  cfg = ConfigurationParser::parse(parameters);
  EXPECT_FALSE(cfg.isDebugSet);
}

TEST(ConfigurationParserTest, should_throw_on_timeout_parsing_error) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_timeout");
  var->set_value("abc");

  try {
    Configuration cfg = ConfigurationParser::parse(parameters);
    FAIL() << "Expected std::invalid_argument.";
  } catch (const std::invalid_argument&) {
    SUCCEED() << "Thrown invalid_argument.";
  } catch (...) {
    FAIL() << "Expected std::invalid_argument.";
  }
}

TEST(ConfigurationParserTest, should_throw_on_timeout_out_of_range) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_timeout");
  var->set_value("10000000000000000000000000000000000000000000000000000");

  try {
    Configuration cfg = ConfigurationParser::parse(parameters);
    FAIL() << "Expected std::out_of_range.";
  } catch (const std::out_of_range&) {
    SUCCEED() << "Thrown out_of_range.";
  } catch (...) {
    FAIL() << "Expected std::out_of_range.";
  }
}

