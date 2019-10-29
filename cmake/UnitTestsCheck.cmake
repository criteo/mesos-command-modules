set(TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/tests/CommandHookTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandIsolatorTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandRunnerTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ConfigurationParserTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ModulesFactoryTest.cpp
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

  PRIVATE ${GLOG_LIBRARY_DIRS}
  PRIVATE ${GTEST_LIBRARY_DIRS}
  PRIVATE ${MESOS_BUILD_DIR}/3rdparty/libprocess/src/
  PRIVATE ${MESOS_BUILD_DIR}/src/
  PRIVATE ${PROTOBUF_LIBRARY_DIRS}
  )

target_link_libraries(${TEST_BINARY_NAME}
  ${PROJECT_NAME}
  glog
  gmock
  mesos-protobufs
  process
  protobuf
  pthread
  )
add_custom_target(check COMMAND "${TEST_BINARY_NAME}")
