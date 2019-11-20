include(PackageFindingHelper)

set(PACKAGE_NAME PICOJSON)

set(
  ${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/picojson*"
  "${MESOS_ROOT_DIR}/3rdparty/picojson*"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

package_finding_helper(${PACKAGE_NAME} picojson.h)
