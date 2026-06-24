cmake_minimum_required(VERSION 3.14)
project(static_string LANGUAGES CXX)

add_library(static_string INTERFACE)
target_include_directories(static_string INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(static_string INTERFACE cxx_std_20)
# Namespaced alias so consumers link `static_string::static_string` whether they
# add this via FetchContent or find_package.
add_library(static_string::static_string ALIAS static_string)

# Only build the smoke test when this repo is the top-level project, so
# consumers (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(static_string_test test/test.cc)
  target_link_libraries(static_string_test PRIVATE static_string)
  add_test(NAME static_string COMMAND static_string_test)
endif()
