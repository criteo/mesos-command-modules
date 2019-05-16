#ifndef __GTEST_HELPERS_HPP__
#define __GTEST_HELPERS_HPP__

#include <future>
#include <regex>
#include <stout/os.hpp>
#include <stout/proc.hpp>
#include <stout/try.hpp>

#define TEST_TIMEOUT_BEGIN                           \
  std::promise<bool> promisedFinished;               \
  auto futureResult = promisedFinished.get_future(); \
                              std::thread([](std::promise<bool>& finished) {
#define TEST_TIMEOUT_FAIL_END(X)                                            \
  finished.set_value(true);                                                 \
  }, std::ref(promisedFinished)).detach(); \
  EXPECT_TRUE(futureResult.wait_for(std::chrono::milliseconds(X)) !=        \
              std::future_status::timeout);

#define TEST_TIMEOUT_SUCCESS_END(X)                                            \
  finished.set_value(true);                                                    \
  }, std::ref(promisedFinished)).detach(); \
  EXPECT_FALSE(futureResult.wait_for(std::chrono::milliseconds(X)) !=          \
               std::future_status::timeout);

#define EXPECT_ERROR_MESSAGE(actual, expectedMessage) \
  EXPECT_TRUE(actual.isError());                      \
  EXPECT_TRUE(AssertRegexMatch(actual.error(), expectedMessage));

::testing::AssertionResult AssertRegexMatch(const std::string& actual,
                                            const std::regex& expected) {
  if (std::regex_match(actual, expected)) return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
         << "\"" << actual << "\" doesn't match regex";
}

#define EXPECT_PROCESS_EXITED(pidfile) \
  EXPECT_TRUE(AssertProcessExited(pidfile));

::testing::AssertionResult AssertProcessExited(const std::string& pidFile) {
  Try<std::string> contents = os::read(pidFile);
  if (contents.isError())
    return ::testing::AssertionFailure()
           << "can't read pid file \"" << pidFile << "\": " << contents.error();
  pid_t pid = std::stoi(contents.get());
  if (proc::status(pid).isSome())
    return ::testing::AssertionFailure()
           << "process " << pid << " is still running";
  return ::testing::AssertionSuccess();
}

#endif
