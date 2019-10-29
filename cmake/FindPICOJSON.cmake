include(PackageFindingHelper)

set(PACKAGE_NAME PICOJSON)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/picojson*"
  )

package_finding_helper(${PACKAGE_NAME} picojson.h)
