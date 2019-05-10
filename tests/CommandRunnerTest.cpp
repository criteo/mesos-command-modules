#include <gtest/gtest.h>
#include <process/gtest.hpp>

#include "CommandRunner.hpp"
#include "gtest_helpers.hpp"

#include <stout/gtest.hpp>
#include <regex>
#include <memory>

using std::string;

using namespace criteo::mesos;
using namespace process;

extern string g_resourcesPath;

class CommandRunnerTest : public ::testing::Test {
public:
  virtual ~CommandRunnerTest() {}

  void SetUp() {
    m_metadata = createMetada();
    m_commandRunner.reset(new CommandRunner(false, m_metadata));
  }

  static logging::Metadata createMetada() {
      logging::Metadata m{"ABC-DEF-GHI", "method"};
      return m;
  }

  logging::Metadata m_metadata;
  std::unique_ptr<CommandRunner> m_commandRunner;
};

TEST_F(CommandRunnerTest, should_run_a_simple_sh_command_and_get_the_output) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "pipe_input.sh", 10), "HELLO");
  EXPECT_EQ(output.get(), "HELLO > output");
}

TEST_F(CommandRunnerTest, should_not_capture_stdout) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "echo_stdout.sh", 10), "HELLO");
  EXPECT_TRUE(output.get().empty());
}

TEST_F(CommandRunnerTest, should_SIGTERM_inifinite_loop_command) {
  logging::Metadata metadata = CommandRunnerTest::createMetada();
  Future<Try<string>> output =
      CommandRunner(false, metadata).asyncRun(Command(g_resourcesPath + "infinite_loop.sh", 2), "");
  AWAIT_ASSERT_READY_FOR(output, Seconds(4000));
  EXPECT_ERROR(output.get());
}

TEST_F(CommandRunnerTest, should_force_SIGKILL_inifinite_loop_command) {
  logging::Metadata metadata = CommandRunnerTest::createMetada();
  Future<Try<string>> output =
      CommandRunner(false, metadata)
          .run(Command(g_resourcesPath + "force_kill.sh", 2), "");
  AWAIT_ASSERT_READY_FOR(output, Seconds(40000));
  EXPECT_ERROR(output.get());
}

TEST_F(CommandRunnerTest, should_not_crash_when_child_throws) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "throw.sh", 10), "");
  EXPECT_ERROR(output);
  EXPECT_ERROR_MESSAGE(
      output, std::regex("Command \".*throw.sh\" exited with return code 1."));
}

TEST_F(CommandRunnerTest, should_not_crash_when_killed_by_signal) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "autokill.sh", 10), "");
  EXPECT_ERROR(output);
  EXPECT_ERROR_MESSAGE(
      output, std::regex("Command \".*autokill.sh\" exited via signal 15."));
}

TEST_F(CommandRunnerTest, should_not_return_error_when_script_works) {
  EXPECT_SOME(m_commandRunner->run(Command(g_resourcesPath + "ok.sh", 10), ""));
}

TEST_F(CommandRunnerTest, should_not_crash_when_executing_unexisting_command) {
  EXPECT_NO_THROW({ m_commandRunner->run(Command("blablabla", 10), ""); });
}

TEST_F(CommandRunnerTest,
     should_return_an_error_when_executing_unexisting_command) {
  Try<string> output = m_commandRunner->run(Command("blablabla", 10), "");
  EXPECT_ERROR(output);
}

TEST_F(CommandRunnerTest,
     should_return_an_error_when_executing_unexecutable_file) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "unexecutable.sh", 10), "");
  EXPECT_ERROR(output);
}

TEST_F(CommandRunnerTest,
     should_return_an_error_with_cause_from_command) {
  Try<string> output =
      m_commandRunner->run(Command(g_resourcesPath + "stderr.sh", 10), "");
  EXPECT_ERROR(output);
  EXPECT_ERROR_MESSAGE(output,
                       std::regex("Command \".*stderr.sh\" exited with return code 1\\. Cause: This is the cause\\."));
}
