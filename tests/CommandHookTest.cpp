#include "CommandHook.hpp"

#include <gtest/gtest.h>

extern std::string g_resourcesPath;

using namespace criteo::mesos;

class CommandHookTest : public ::testing::Test {
 protected:
  mesos::TaskInfo taskInfo;
  mesos::ExecutorInfo executorInfo;
  mesos::FrameworkInfo frameworkInfo;
  mesos::SlaveInfo slaveInfo;

 public:
  void SetUp() {
    hook.reset(new CommandHook(
        Command(g_resourcesPath + "slaveRunTaskLabelDecorator.sh"),
        Command(g_resourcesPath + "slaveExecutorEnvironmentDecorator.sh"),
        Command(g_resourcesPath + "slaveRemoveExecutorHook.sh")));
    taskInfo.set_name("test_task");
    taskInfo.mutable_task_id()->set_value("1");
    taskInfo.mutable_slave_id()->set_value("2");
    {
      auto l = taskInfo.mutable_labels()->add_labels();
      l->set_key("foo");
      l->set_value("bar");
    }

    executorInfo.set_name("test_exec");
    {
      auto env = executorInfo.mutable_command()->mutable_environment();
      auto var = env->add_variables();
      var->set_name("foo");
      var->set_value("bar");
      var = env->add_variables();
      var->set_name("deleted");
      var->set_value("whatever");
    }
  }

  std::unique_ptr<CommandHook> hook;
};

TEST_F(CommandHookTest,
       should_run_slaveRunTaskLabelDecorator_command_and_retrieve_labels) {
  auto result = hook->slaveRunTaskLabelDecorator(taskInfo, executorInfo,
                                                 frameworkInfo, slaveInfo);
  ASSERT_TRUE(result.isSome());
  auto labels = result.get();
  ASSERT_EQ(labels.labels_size(), 2);

  ASSERT_EQ(labels.labels(0).key(), "LABEL_1");
  ASSERT_EQ(labels.labels(0).value(), "test1");
  ASSERT_EQ(labels.labels(1).key(), "LABEL_2");
  ASSERT_EQ(labels.labels(1).value(), "test2");
}

TEST_F(
    CommandHookTest,
    should_run_slaveExecutorEnvironmentDecorator_command_and_retrieve_environment_variables) {
  auto result = hook->slaveExecutorEnvironmentDecorator(executorInfo);
  ASSERT_TRUE(result.isSome());
  auto environment = result.get();
  ASSERT_EQ(environment.variables_size(), 2);

  ASSERT_EQ(environment.variables(0).name(), "ENV_1");
  ASSERT_EQ(environment.variables(0).value(), "test1");
  ASSERT_EQ(environment.variables(1).name(), "ENV_2");
  ASSERT_EQ(environment.variables(1).value(), "test2");
}

TEST_F(CommandHookTest, should_run_slaveRemoveExecutorHook_command) {
  hook->slaveRemoveExecutorHook(frameworkInfo, executorInfo);
}

class UnexistingCommandHookTest : public CommandHookTest {
 public:
  void SetUp() {
    CommandHookTest::SetUp();
    hook.reset(
        new CommandHook(Command("unexisting.sh"), Command("unexisting.sh"), Command("unexisting.sh")));
  }
  std::unique_ptr<CommandHook> hook;
};

TEST_F(
    UnexistingCommandHookTest,
    should_run_slaveRunTaskLabelDecorator_command_and_handle_unexisting_command) {
  auto result = hook->slaveRunTaskLabelDecorator(taskInfo, executorInfo,
                                                 frameworkInfo, slaveInfo);
  ASSERT_TRUE(result.isError());
}

TEST_F(
    UnexistingCommandHookTest,
    should_run_slaveExecutorEnvironmentDecorator_command_and_handle_unexisting_command) {
  auto result = hook->slaveExecutorEnvironmentDecorator(executorInfo);
  ASSERT_TRUE(result.isError());
}

class MalformedCommandHookTest : public CommandHookTest {
 public:
  void SetUp() {
    CommandHookTest::SetUp();
    hook.reset(new CommandHook(
        Command(g_resourcesPath + "slaveRunTaskLabelDecorator_malformed.sh"),
        Command(g_resourcesPath + "slaveExecutorEnvironmentDecorator_malformed.sh"),
        Command(g_resourcesPath + "slaveRemoveExecutorHook_malformed.sh")));
  }
  std::unique_ptr<CommandHook> hook;
};

TEST_F(
    MalformedCommandHookTest,
    should_run_slaveRunTaskLabelDecorator_command_and_handle_malformed_output_json) {
  auto result = hook->slaveRunTaskLabelDecorator(taskInfo, executorInfo,
                                                 frameworkInfo, slaveInfo);
  ASSERT_TRUE(result.isError());
}

TEST_F(
    MalformedCommandHookTest,
    should_run_slaveExecutorEnvironmentDecorator_command_and_handle_malformed_output_json) {
  auto result = hook->slaveExecutorEnvironmentDecorator(executorInfo);
  ASSERT_TRUE(result.isError());
}

class EmptyCommandHookTest : public CommandHookTest {
 public:
  void SetUp() {
    CommandHookTest::SetUp();
    hook.reset(new CommandHook(None(), None(), None()));
  }
  std::unique_ptr<CommandHook> hook;
};

TEST_F(EmptyCommandHookTest,
       should_run_slaveRunTaskLabelDecorator_command_and_retrieve_labels) {
  auto result = hook->slaveRunTaskLabelDecorator(taskInfo, executorInfo,
                                                 frameworkInfo, slaveInfo);
  ASSERT_TRUE(result.isNone());
}

TEST_F(
    EmptyCommandHookTest,
    should_run_slaveExecutorEnvironmentDecorator_command_and_retrieve_environment_variables) {
  auto result = hook->slaveExecutorEnvironmentDecorator(executorInfo);
  ASSERT_TRUE(result.isNone());
}

class IncorrectProtobufCommandHookTest : public CommandHookTest {
 public:
  void SetUp() {
    CommandHookTest::SetUp();
    hook.reset(new CommandHook(
        Command(g_resourcesPath + "slaveRunTaskLabelDecorator_incorrect_protobuf.sh"),
        Command(g_resourcesPath +
            "slaveExecutorEnvironmentDecorator_incorrect_protobuf.sh"),
        Command(g_resourcesPath + "slaveRemoveExecutorHook_incorrect_protobuf.sh")));
  }
  std::unique_ptr<CommandHook> hook;
};

TEST_F(
    IncorrectProtobufCommandHookTest,
    should_run_slaveRunTaskLabelDecorator_command_and_handle_incorrect_protobuf_output) {
  auto result = hook->slaveRunTaskLabelDecorator(taskInfo, executorInfo,
                                                 frameworkInfo, slaveInfo);
  ASSERT_TRUE(result.isError());
}

TEST_F(
    IncorrectProtobufCommandHookTest,
    should_run_slaveExecutorEnvironmentDecorator_command_and_handle_incorrect_protobuf_output) {
  auto result = hook->slaveExecutorEnvironmentDecorator(executorInfo);
  ASSERT_TRUE(result.isError());
}
