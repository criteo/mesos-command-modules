include(PackageFindingHelper)

set(PACKAGE_NAME GTEST)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/googletest*"
  )

set(${PACKAGE_NAME}_LIBRARY_NAMES libgmock.a)

package_finding_helper(${PACKAGE_NAME} gtest/gtest.h)
