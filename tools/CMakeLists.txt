cmake_minimum_required(VERSION 2.8)

project(taishell-tools)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -std=gnu99")

include_directories(
  ../include
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(mktai
  mktai.c
  crc32.c
)

install(TARGETS mktai
  RUNTIME DESTINATION bin
)