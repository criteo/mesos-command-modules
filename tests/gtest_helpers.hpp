#ifndef __GTEST_HELPERS_HPP__
#define __GTEST_HELPERS_HPP__

#include <future>
#include <regex>

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

#endif
