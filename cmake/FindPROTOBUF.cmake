include(PackageFindingHelper)

set(PACKAGE_NAME PROTOBUF)

subdirlist(
  ${PACKAGE_NAME}_SRCH_DIRS
  "${MESOS_BUILD_DIR}/3rdparty/protobuf*"
  )

set(${PACKAGE_NAME}_LIBRARY_NAMES libprotobuf.so)

package_finding_helper(${PACKAGE_NAME} google/protobuf/stubs/common.h)
