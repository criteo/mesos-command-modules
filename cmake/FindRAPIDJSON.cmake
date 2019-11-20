include(PackageFindingHelper)

set(PACKAGE_NAME RAPIDJSON)

set(
  ${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/rapidjson*"
  "${MESOS_ROOT_DIR}/3rdparty/rapidjson*"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

package_finding_helper(${PACKAGE_NAME} rapidjson/stringbuffer.h)
