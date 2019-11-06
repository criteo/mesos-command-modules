#ifndef __GTEST_HELPERS_HPP__
#define __GTEST_HELPERS_HPP__

#include <future>
#include <gtest/gtest.h>
#include <process/gtest.hpp>
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
                                            const std::regex& expected);

#define EXPECT_PROCESS_EXITED(pidfile) \
  EXPECT_TRUE(AssertProcessExited(pidfile));

::testing::AssertionResult AssertProcessExited(const std::string& pidFile);

#define AWAIT_ASSERT_PENDING_FOR(actual, duration) \
  ASSERT_PRED_FORMAT2(AwaitAssertPending, actual, duration)

#define AWAIT_ASSERT_PENDING(actual) \
  AWAIT_ASSERT_PENDING_FOR(actual, process::TEST_AWAIT_TIMEOUT)

#define AWAIT_EXPECT_PENDING_FOR(actual, duration) \
  EXPECT_PRED_FORMAT2(AwaitAssertPending, actual, duration)

#define AWAIT_EXPECT_PENDING(actual) \
  AWAIT_EXPECT_PENDING_FOR(actual, process::TEST_AWAIT_TIMEOUT)

#include "gtest_helpers.hxx"
#endif  // __GTEST_HELPERS_HPP__
