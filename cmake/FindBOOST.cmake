include(PackageFindingHelper)

set(PACKAGE_NAME BOOST)

set(${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/boost*"
  "${MESOS_ROOT_DIR}/3rdparty/boost*"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

package_finding_helper(${PACKAGE_NAME} boost/version.hpp)
