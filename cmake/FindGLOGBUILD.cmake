include(PackageFindingHelper)

set(PACKAGE_NAME GLOGBUILD)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/glog*"
  )

package_finding_helper(${PACKAGE_NAME} glog/logging.h)
