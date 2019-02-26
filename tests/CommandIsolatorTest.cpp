#include "CommandIsolator.hpp"

#include <gtest/gtest.h>
#include <process/gtest.hpp>

extern std::string g_resourcesPath;

using namespace criteo::mesos;
using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;
using ::mesos::slave::ContainerLimitation;

class CommandIsolatorTest : public ::testing::Test {
 protected:
  ContainerID containerId;
  ContainerConfig containerConfig;

 public:
  void SetUp() {
    isolator.reset(
        new CommandIsolator(Command(g_resourcesPath + "prepare.sh"),
                            Command(g_resourcesPath + "watch.sh"),
                            Command(g_resourcesPath + "cleanup.sh")));
    containerId.set_value("container_id");

    containerConfig.set_rootfs("/isolated_fs");
    containerConfig.set_user("app_user");
  }

  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(CommandIsolatorTest,
       should_run_prepare_command_and_retrieve_container_launch_info) {
  auto containerLaunchInfoFuture =
      isolator->prepare(containerId, containerConfig);

  AWAIT_READY(containerLaunchInfoFuture);

  Option<ContainerLaunchInfo> launchInfo = containerLaunchInfoFuture.get();

  EXPECT_EQ("/isolated_fs", launchInfo->rootfs());
  EXPECT_EQ("app_user", launchInfo->user());
}

TEST_F(CommandIsolatorTest,
       should_run_watch_command_and_retrieve_container_limitation) {
  auto containerLimitation = isolator->watch(containerId);

  AWAIT_READY(containerLimitation);

  ContainerLimitation limited = containerLimitation.get();

  EXPECT_EQ("too much toto", limited.message());
}

TEST_F(CommandIsolatorTest,
       should_run_cleanup_command_and_terminates_successfully) {
  auto future = isolator->cleanup(containerId);
  AWAIT_READY(future);
}

class UnexistingCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(Command("unexisting.sh"),
                                       Command("unexisting.sh"),
                                       Command("unexisting.sh")));
  }
  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(UnexistingCommandIsolatorTest,
       should_try_to_run_prepare_command_and_fail) {
  auto future = isolator->prepare(containerId, containerConfig);
  AWAIT_FAILED(future);
}

TEST_F(UnexistingCommandIsolatorTest,
       should_try_to_run_watch_command_and_fail) {
  auto future = isolator->watch(containerId);
  AWAIT_ASSERT_ABANDONED(future);
}

TEST_F(UnexistingCommandIsolatorTest,
       should_try_to_run_cleanup_command_and_fail) {
  auto future = isolator->cleanup(containerId);
  AWAIT_FAILED(future);
}

class MalformedCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        Command(g_resourcesPath + "prepare_malformed.sh"),
        Command(g_resourcesPath + "watch_malformed.sh"), None()));
  }
  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(MalformedCommandIsolatorTest,
       should_run_prepare_command_and_handle_malformed_output_json) {
  auto future = isolator->prepare(containerId, containerConfig);
  AWAIT_FAILED(future);
}

TEST_F(MalformedCommandIsolatorTest,
       should_run_watch_command_and_handle_malformed_output_json) {
  auto future = isolator->watch(containerId);
  AWAIT_ASSERT_ABANDONED(future);
}

class EmptyCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(None(), None(), None()));
  }
  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(
    EmptyCommandIsolatorTest,
    should_resolve_promise_and_return_null_option_when_prepare_command_is_empty) {
  auto future = isolator->prepare(containerId, containerConfig);
  AWAIT_READY(future);
  EXPECT_TRUE(future->isNone());
}

TEST_F(EmptyCommandIsolatorTest,
       should_resolve_promise_when_watch_command_is_empty) {
  auto future = isolator->watch(containerId);
  AWAIT_ASSERT_ABANDONED(future);
}

TEST_F(EmptyCommandIsolatorTest,
       should_resolve_promise_when_cleanup_command_is_empty) {
  auto future = isolator->cleanup(containerId);
  AWAIT_READY(future);
}

class IncorrectProtobufCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        Command(g_resourcesPath + "prepare_incorrect_protobuf.sh"),
        g_resourcesPath + "watch_incorrect_protobuf.sh", None()));
  }
  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(IncorrectProtobufCommandIsolatorTest,
       should_run_prepare_command_and_handle_incorrect_protobuf_output) {
  auto future = isolator->prepare(containerId, containerConfig);
  AWAIT_FAILED(future);
}

TEST_F(IncorrectProtobufCommandIsolatorTest,
       should_run_watch_command_and_handle_incorrect_protobuf_output) {
  auto future = isolator->watch(containerId);
  AWAIT_ASSERT_ABANDONED(future);
}

class EmptyOutputCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        None(), Command(g_resourcesPath + "watch_empty.sh"), None()));
  }
  std::unique_ptr<CommandIsolator> isolator;
};

TEST_F(EmptyOutputCommandIsolatorTest,
       should_run_watch_command_and_do_nothing_on_empty_output) {
  auto future = isolator->watch(containerId);
  AWAIT_ASSERT_ABANDONED(future);
}
