#include "gtest_helpers.hpp"

::testing::AssertionResult AssertRegexMatch(const std::string& actual,
                                            const std::regex& expected) {
  if (std::regex_match(actual, expected)) return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
         << "\"" << actual << "\" doesn't match regex";
}

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
