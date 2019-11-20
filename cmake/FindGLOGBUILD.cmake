include(PackageFindingHelper)

set(PACKAGE_NAME GLOGBUILD)

set(
  ${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/glog*"
  "${MESOS_ROOT_DIR}/3rdparty/glog*"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

package_finding_helper(${PACKAGE_NAME} glog/logging.h)
