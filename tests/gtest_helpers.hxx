#ifndef __GTEST_HELPERS_HXX__
#define __GTEST_HELPERS_HXX__

template <typename T>
::testing::AssertionResult AwaitAssertPending(
    const char* expr,
    const char*,  // Unused string representation of 'duration'.
    const process::Future<T>& actual, const Duration& duration) {
  os::sleep(duration);

  CHECK_PENDING(actual);

  return ::testing::AssertionSuccess();
}

#endif  // __GTEST_HELPERS_HXX__
