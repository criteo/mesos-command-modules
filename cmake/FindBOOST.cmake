include(PackageFindingHelper)

set(PACKAGE_NAME BOOST)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/boost*"
  )

package_finding_helper(${PACKAGE_NAME} boost/version.hpp)
