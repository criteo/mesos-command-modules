include(PackageFindingHelper)

set(PACKAGE_NAME RAPIDJSON)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/rapidjson*"
  )

package_finding_helper(${PACKAGE_NAME} rapidjson/stringbuffer.h)
