#include "CommandIsolator.hpp"
#include "gtest_helpers.hpp"

#include <gtest/gtest.h>
#include <process/gtest.hpp>

extern std::string g_resourcesPath;

using namespace criteo::mesos;
using ::mesos::ContainerID;
using ::mesos::slave::ContainerConfig;
using ::mesos::slave::ContainerLaunchInfo;
using ::mesos::slave::ContainerLimitation;
using ::mesos::slave::ContainerState;

class CommandIsolatorTest : public ::testing::Test {
 protected:
  ContainerID containerId;
  pid_t pid;
  ContainerConfig containerConfig;
  process::Future<Option<ContainerLaunchInfo>> containerLaunchInfoFuture;

 public:
  void SetUp() {
    containerId.set_value("container_id");
    pid = 5000;
    containerConfig.set_rootfs("/isolated_fs");
    containerConfig.set_user("app_user");
  }
  void Prepare() {
      containerLaunchInfoFuture =
        isolator->prepare(containerId, containerConfig);
      AWAIT_READY(containerLaunchInfoFuture);
  }
  void PrepareFailed() {
      containerLaunchInfoFuture =
        isolator->prepare(containerId, containerConfig);
      AWAIT_FAILED(containerLaunchInfoFuture);
    }

  process::Future<Nothing> recoverOneContainer() {
    ContainerState state;
    state.mutable_container_id()->set_value(containerId.value());
    std::vector<ContainerState> states = {state};
    hashset<::mesos::ContainerID> orphans;
    return isolator->recover(states, orphans);
  }

  std::unique_ptr<CommandIsolator> isolator;
};

class CommandIsolatorSimpleTest : public CommandIsolatorTest {
public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(
      new CommandIsolator("test", Command(g_resourcesPath + "prepare.sh"), 
                          Command(g_resourcesPath + "isolate.sh"),
                          RecurrentCommand(g_resourcesPath + "watch.sh", 3, 0.1),
                          Command(g_resourcesPath + "cleanup.sh"),
                          Command(g_resourcesPath + "usage.sh")));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(CommandIsolatorSimpleTest,
       should_run_prepare_command_and_retrieve_container_launch_info) {
  Option<ContainerLaunchInfo> launchInfo = containerLaunchInfoFuture.get();

  EXPECT_EQ("/isolated_fs", launchInfo->rootfs());
  EXPECT_EQ("app_user", launchInfo->user());
}

TEST_F(CommandIsolatorSimpleTest,
       should_run_watch_command_and_retrieve_container_limitation) {
  auto containerLimitation = isolator->watch(containerId);

  AWAIT_READY(containerLimitation);

  ContainerLimitation limited = containerLimitation.get();

  EXPECT_EQ("too much toto", limited.message());
  containerLimitation.discard();
}

TEST_F(CommandIsolatorSimpleTest,
       should_run_cleanup_command_and_terminates_successfully) {
  auto future = isolator->cleanup(containerId);
  AWAIT_READY(future);
}

TEST_F(CommandIsolatorSimpleTest,
       should_run_isolate_command_and_terminate_successfully) {
  auto future = isolator->isolate(containerId, pid);
  AWAIT_READY(future);
}

TEST_F(CommandIsolatorSimpleTest,
       cleanup_should_remove_container_context) {
  AWAIT_READY(isolator->cleanup(containerId));
  ASSERT_FALSE(isolator->hasContainerContext(containerId));
}

TEST_F(CommandIsolatorSimpleTest,
       should_run_usage_command_and_retrieve_container_limitation) {
  auto resourceStatistics = isolator->usage(containerId);

  AWAIT_READY(resourceStatistics);

  ::mesos::ResourceStatistics stats = resourceStatistics.get();

  EXPECT_EQ(5, stats.net_snmp_statistics().tcp_stats().currestab());
}

TEST_F(CommandIsolatorSimpleTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class CommandIsolatorContinuousTest : public CommandIsolatorTest {
public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(
      new CommandIsolator("test", Command(g_resourcesPath + "prepare.sh"),
                          Command(g_resourcesPath + "isolate.sh"),
                          RecurrentCommand(g_resourcesPath + "watch_continuous.sh", 3, 0.1),
                          Command(g_resourcesPath + "cleanup.sh"),
                          Command(g_resourcesPath + "usage_continuous.sh")));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(CommandIsolatorContinuousTest,
  should_run_prepare_isolate_watch_usage_and_cleanup_commands) {

  auto isolate_future = isolator->isolate(containerId, pid);
  AWAIT_READY(isolate_future);

  auto containerLimitation = isolator->watch(containerId);
  AWAIT_READY(containerLimitation);
  ContainerLimitation limited = containerLimitation.get();
  EXPECT_EQ("user found", limited.message());

  auto resourceStatistics = isolator->usage(containerId);
  AWAIT_READY(resourceStatistics);
  ::mesos::ResourceStatistics stats = resourceStatistics.get();
  EXPECT_EQ(1, stats.timestamp());

  auto cleanup_future = isolator->cleanup(containerId);
  AWAIT_READY(cleanup_future);

}

class UnexistingCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator("test", Command("unexisting.sh"),
                                       Command("unexisting.sh"),
                                       RecurrentCommand("unexisting.sh", 1, 0.1),
                                       Command("unexisting.sh"),
                                       Command("unexisting.sh")
                                       ));
    CommandIsolatorTest::PrepareFailed();
  }
};

TEST_F(UnexistingCommandIsolatorTest,
       should_try_to_run_watch_command_and_fail) {
  auto future = isolator->watch(containerId);
  AWAIT_EXPECT_PENDING_FOR(future, Seconds(1));
  future.discard();
}

TEST_F(UnexistingCommandIsolatorTest,
       should_try_to_run_isolate_command_and_fail) {
  auto future = isolator->isolate(containerId, pid);
  AWAIT_FAILED(future);
}

TEST_F(UnexistingCommandIsolatorTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class NotPreparedCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator("test", Command("unexisting.sh"),
                                       Command("unexisting.sh"),
                                       RecurrentCommand("unexisting.sh", 1, 0.1),
                                       Command("unexisting.sh"),
                                       Command("unexisting.sh")
                                       ));
  }
};

TEST_F(NotPreparedCommandIsolatorTest,
       should_not_run_cleanup_command_and_succeed) {
  auto future = isolator->cleanup(containerId);
  AWAIT_READY(future);
}

TEST_F(NotPreparedCommandIsolatorTest,
       should_not_run_isolate_command_and_fail) {
  auto future = isolator->isolate(containerId, pid);
  AWAIT_FAILED(future);
}

TEST_F(NotPreparedCommandIsolatorTest,
       should_fail_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_FALSE(isolator->hasContainerContext(containerId));
}

class MalformedCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        "test",
        Command(g_resourcesPath + "prepare_malformed.sh"),
        None(),
        RecurrentCommand(g_resourcesPath + "watch_malformed.sh", 1, 0.3),
        None(),
        Command(g_resourcesPath + "usage_malformed.sh")
        ));
    CommandIsolatorTest::PrepareFailed();
  }
};

TEST_F(MalformedCommandIsolatorTest,
       should_run_watch_command_and_handle_malformed_output_json) {
  auto future = isolator->watch(containerId);
  AWAIT_EXPECT_PENDING_FOR(future, Seconds(1));
  future.discard();
}

TEST_F(MalformedCommandIsolatorTest,
       should_run_usage_command_and_handle_malformed_output_json) {
  auto resourceStatistics = isolator->usage(containerId);

  AWAIT_READY(resourceStatistics);

  ::mesos::ResourceStatistics stats = resourceStatistics.get();
}

TEST_F(MalformedCommandIsolatorTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class EmptyCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator("test", None(), None(), None(), None(), None()));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(
    EmptyCommandIsolatorTest,
    should_resolve_promise_and_return_null_option_when_prepare_command_is_empty) {
  EXPECT_TRUE(containerLaunchInfoFuture->isNone());
}

TEST_F(EmptyCommandIsolatorTest,
       should_resolve_promise_when_watch_command_is_empty) {
  auto future = isolator->watch(containerId);
  AWAIT_EXPECT_PENDING_FOR(future, Seconds(1));
  future.discard();
}

TEST_F(EmptyCommandIsolatorTest,
       should_resolve_promise_when_cleanup_command_is_empty) {
  auto future = isolator->cleanup(containerId);
  AWAIT_READY(future);
}

TEST_F(EmptyCommandIsolatorTest,
       should_resolve_promise_when_usage_command_is_empty) {
  auto resourceStatistics = isolator->usage(containerId);

  AWAIT_READY(resourceStatistics);

  ::mesos::ResourceStatistics stats = resourceStatistics.get();
}

TEST_F(EmptyCommandIsolatorTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class IncorrectProtobufCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        "test",
        Command(g_resourcesPath + "prepare_incorrect_protobuf.sh"),
        None(),
        RecurrentCommand(g_resourcesPath + "watch_incorrect_protobuf.sh", 3, 0.1),
        None(),
        Command(g_resourcesPath + "usage_incorrect_protobuf.sh")
        ));
    CommandIsolatorTest::PrepareFailed();
  }
};

TEST_F(IncorrectProtobufCommandIsolatorTest,
       should_run_watch_command_and_handle_incorrect_protobuf_output) {
  auto future = isolator->watch(containerId);
  AWAIT_EXPECT_PENDING_FOR(future, Seconds(1));
  future.discard();
}

TEST_F(IncorrectProtobufCommandIsolatorTest,
       should_run_usage_command_and_handle_incorrect_protobuf_output) {
  auto resourceStatistics = isolator->usage(containerId);

  AWAIT_READY(resourceStatistics);

  ::mesos::ResourceStatistics stats = resourceStatistics.get();
}

TEST_F(IncorrectProtobufCommandIsolatorTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class EmptyOutputCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        "test",
        None(), None(), RecurrentCommand(g_resourcesPath + "watch_empty.sh", 3, 0.1), None(),
        Command(g_resourcesPath + "usage_empty.sh")
        ));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(EmptyOutputCommandIsolatorTest,
       should_run_watch_command_and_do_nothing_on_empty_output) {
  auto future = isolator->watch(containerId);
  AWAIT_EXPECT_PENDING_FOR(future, Seconds(1));
  future.discard();
}

TEST_F(EmptyOutputCommandIsolatorTest, should_return_empty_stats_on_empty_usage) {
  auto stats = isolator->usage(containerId);
  AWAIT_READY(stats);
  EXPECT_EQ(0, stats.get().cpus_system_time_secs());
  EXPECT_TRUE(stats.get().has_timestamp());
}

TEST_F(EmptyOutputCommandIsolatorTest,
       should_recover_container) {
  AWAIT_READY(recoverOneContainer());
  ASSERT_TRUE(isolator->hasContainerContext(containerId));
}

class TimeoutCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        "test",
        None(), None(), None(), None(),
        Command(g_resourcesPath + "usage_timeout.sh", 1)
        ));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(TimeoutCommandIsolatorTest, should_return_empty_stats_on_usage_timeout) {
  auto stats = isolator->usage(containerId);
  AWAIT_ASSERT_READY_FOR(stats, Seconds(4));
  EXPECT_EQ(0, stats.get().cpus_system_time_secs());
  EXPECT_TRUE(stats.get().has_timestamp());
}

class LongCommandIsolatorTest : public CommandIsolatorTest {
 public:
  void SetUp() {
    CommandIsolatorTest::SetUp();
    isolator.reset(new CommandIsolator(
        "test",
        None(), None(), None(), None(),
        Command(g_resourcesPath + "usage_long.sh", 1)
        ));
    CommandIsolatorTest::Prepare();
  }
};

TEST_F(LongCommandIsolatorTest,
       should_run_usage_command_and_does_not_crash_if_discarded) {
  auto future = isolator->usage(containerId);
  future.discard();
  AWAIT_ASSERT_READY_FOR(future, Seconds(3));
}
