#include <gtest/gtest.h>
#include "CommandRunner.hpp"

using namespace criteo::mesos;

TEST(CommandRunnerTest, should_run_a_simple_sh_command_and_get_the_output) {
  std::string output = CommandRunner::run("/bin/sh -c 'cat $0 > $1'", "HELLO");
  ASSERT_EQ(output, "HELLO");
}

TEST(CommandRunnerTest, should_not_crash_when_child_throws) {
  EXPECT_NO_THROW({
    std::string output = CommandRunner::run("/bin/sh -c 'exit 1'", "");
    ASSERT_EQ(output, "");
  });
}

TEST(CommandRunnerTest, should_not_crash_when_executing_unexisting_command) {
  EXPECT_NO_THROW({
    std::string output = CommandRunner::run("blablabla", "");
    ASSERT_EQ(output, "");
  });
}
