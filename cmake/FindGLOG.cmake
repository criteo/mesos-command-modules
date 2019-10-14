include(PackageFindingHelper)

set(PACKAGE_NAME GLOG)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/glog*/"
  )

set(${PACKAGE_NAME}_LIBRARY_NAMES libglog.so)

package_finding_helper(${PACKAGE_NAME} glog/log_severity.h)
