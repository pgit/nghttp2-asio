#
# Google Test: Avoid warnings in CMake 3.24.0:
#
if (CMAKE_VERSION VERSION_GREATER "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

find_package(fmt)

include_directories(
  "${CMAKE_SOURCE_DIR}/lib/includes"
  "${CMAKE_SOURCE_DIR}/lib"
  "${CMAKE_SOURCE_DIR}/src/includes"
  "${CMAKE_BINARY_DIR}/lib/includes"
  ${CUNIT_INCLUDE_DIRS}
)

enable_testing()

add_executable(tests hello.cc)
target_link_libraries(tests GTest::gtest_main)
target_link_libraries(tests nghttp2_asio)
target_link_libraries(tests fmt::fmt)

include(GoogleTest)
gtest_discover_tests(tests)
