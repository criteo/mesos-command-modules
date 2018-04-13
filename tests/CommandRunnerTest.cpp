#include <gtest/gtest.h>
#include "CommandRunner.hpp"
#include "gtest_helpers.hpp"

extern std::string g_resourcesPath;

using namespace criteo::mesos;

TEST(CommandRunnerTest, should_run_a_simple_sh_command_and_get_the_output) {
  std::string output = CommandRunner::run(g_resourcesPath + "pipe_input.sh",
    "HELLO");
  ASSERT_EQ(output, "HELLO");
}

TEST(CommandRunnerTest, should_SIGTERM_inifinite_loop_command) {
  TEST_TIMEOUT_BEGIN
  std::string output = CommandRunner::run(g_resourcesPath + "infinite_loop.sh", "", 1);
  ASSERT_EQ(output, "");
  TEST_TIMEOUT_FAIL_END(2000)
}

TEST(CommandRunnerTest, should_force_SIGKILL_inifinite_loop_command) {
  TEST_TIMEOUT_BEGIN
  std::string output = CommandRunner::run(g_resourcesPath + "force_kill.sh", "", 1);
  ASSERT_EQ(output, "");
  TEST_TIMEOUT_FAIL_END(3000)
}

TEST(CommandRunnerTest, should_not_crash_when_child_throws) {
  EXPECT_NO_THROW({
    std::string output = CommandRunner::run(g_resourcesPath + "throw.sh", "");
    ASSERT_EQ(output, "");
  });
}

TEST(CommandRunnerTest, should_not_crash_when_executing_unexisting_command) {
  EXPECT_NO_THROW({
    std::string output = CommandRunner::run("blablabla", "");
    ASSERT_EQ(output, "");
  });
}
