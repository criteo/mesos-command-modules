set(TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/tests/CommandHookTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandIsolatorTest.cpp 
  ${CMAKE_SOURCE_DIR}/tests/CommandResourceEstimatorTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandRunnerTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ConfigurationParserTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ModulesFactoryTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/gtest_helpers.cpp
  ${CMAKE_SOURCE_DIR}/tests/main.cpp
)

SET(TEST_BINARY_NAME test_mesos_command_modules)
add_executable(${TEST_BINARY_NAME}
  ${TEST_SOURCES}
)

target_include_directories(
  ${TEST_BINARY_NAME}

  PUBLIC ${GTEST_INCLUDE_DIR} tests
  )
target_link_directories(
  ${TEST_BINARY_NAME}

  PRIVATE ${MESOS_BUILD_DIR}/3rdparty/libprocess/src/
  PRIVATE ${MESOS_ROOT_DIR}/3rdparty/libprocess/.libs/
  PRIVATE ${MESOS_BUILD_DIR}/src/
  )

find_library(
    MESOS-PROTOBUFS_LIBRARY
    NAMES mesos-protobufs
    HINTS ${MESOS_BUILD_DIR}/src/
    NO_DEFAULT_PATH
  )

if(NOT MESOS-PROTOBUFS_LIBRARY)
  unset(MESOS-PROTOBUFS_LIBRARY CACHE)
endif()

target_link_libraries(${TEST_BINARY_NAME}
  ${PROJECT_NAME}
  ${GLOG_LIBRARY}
  ${GTEST_LIBRARY}
  ${PROTOBUF_LIBRARY}
  ${MESOS-PROTOBUFS_LIBRARY}
  process
  pthread
  )
add_custom_target(check COMMAND "${TEST_BINARY_NAME}")
