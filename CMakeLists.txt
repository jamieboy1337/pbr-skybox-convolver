cmake_minimum_required(VERSION 3.15.0)
project(pbr-skybox-convolver VERSION 0.0.1)

include(CTest)
enable_testing()

set(INC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_library(hdr
  ${SRC_DIR}/HDR.cpp)

target_include_directories(hdr PUBLIC ${INC_DIR})

message(${CMAKE_TOOLCHAIN_FILE})

find_package(glm CONFIG REQUIRED)

target_link_libraries(hdr PRIVATE glm)
add_executable(hdr-test 
  ${TEST_DIR}/HDRReadTest.cpp)

target_link_libraries(hdr-test PRIVATE glm hdr)

if(MSVC)
  target_compile_options(hdr PRIVATE /W3)
  target_compile_options(hdr-test PRIVATE /W3)
else()
  target_compile_options(hdr PRIVATE -Wall)
  target_compile_options(hdr-test PRIVATE -Wall)
endif()

target_include_directories(hdr-test PUBLIC ${INC_DIR})