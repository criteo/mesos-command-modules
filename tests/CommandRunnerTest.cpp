#include <gtest/gtest.h>

#include "CommandRunner.hpp"
#include "gtest_helpers.hpp"

#include <stout/gtest.hpp>

using std::string;

using namespace criteo::mesos;

extern string g_resourcesPath;


TEST(CommandRunnerTest, should_run_a_simple_sh_command_and_get_the_output) {
  Try<string> output = CommandRunner::run(g_resourcesPath + "pipe_input.sh",
    "HELLO");
  EXPECT_EQ(output.get(), "HELLO");
}

TEST(CommandRunnerTest, should_SIGTERM_inifinite_loop_command) {
  TEST_TIMEOUT_BEGIN
  Try<string> output = CommandRunner::run(
    g_resourcesPath + "infinite_loop.sh", "", 1);
  EXPECT_ERROR(output);
  TEST_TIMEOUT_FAIL_END(2000)
}

TEST(CommandRunnerTest, should_force_SIGKILL_inifinite_loop_command) {
  TEST_TIMEOUT_BEGIN
  Try<string> output = CommandRunner::run(
    g_resourcesPath + "force_kill.sh", "", 1);
  EXPECT_ERROR(output);
  TEST_TIMEOUT_FAIL_END(3000)
}

TEST(CommandRunnerTest, should_not_crash_when_child_throws) {
  EXPECT_NO_THROW({
    CommandRunner::run(g_resourcesPath + "throw.sh", "");
  });
}

TEST(CommandRunnerTest, should_not_crash_when_executing_unexisting_command) {
  EXPECT_NO_THROW({
    CommandRunner::run("blablabla", "");
  });
}

TEST(CommandRunnerTest, should_return_an_error_when_executing_unexisting_command) {
  Try<string> output = CommandRunner::run("blablabla", "");
  EXPECT_ERROR(output);
}

TEST(CommandRunnerTest, should_return_an_error_when_executing_unexecutable_file) {
  Try<string> output = CommandRunner::run(
      g_resourcesPath + "unexecutable.sh", "");
  EXPECT_ERROR(output);
}
