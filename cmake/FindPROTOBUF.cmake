include(PackageFindingHelper)

set(PACKAGE_NAME PROTOBUF)

set(
  ${PACKAGE_NAME}_SRCH_GLOBS
  "${MESOS_BUILD_DIR}/3rdparty/protobuf*"
  "${MESOS_ROOT_DIR}/3rdparty/protobuf*"
  )

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  ${PACKAGE_NAME}_SRCH_GLOBS
  )

set(${PACKAGE_NAME}_LIBRARY_NAMES protobuf)

package_finding_helper(${PACKAGE_NAME} google/protobuf/stubs/common.h)
