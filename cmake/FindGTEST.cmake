include(PackageFindingHelper)

set(PACKAGE_NAME GTEST)

set(
  ${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/googletest*"
  "${MESOS_ROOT_DIR}/3rdparty/googletest*"
  "${MESOS_ROOT_DIR}/3rdparty/.libs"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

set(${PACKAGE_NAME}_LIBRARY_NAMES gmock)

package_finding_helper(${PACKAGE_NAME} gtest/gtest.h)
