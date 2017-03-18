cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(taishell)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
  ../include
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/taishell-stubs
)

add_subdirectory(plugins plugins)

add_definitions(
  -DTAISHELL_PATH="ux0:taishell"
  -DPLUGINS_DIR="plugins"
  -DPATH_MAX=128
)

add_executable(taishell
  taishell.c
  plugins.c
  logger.c
  list.c
  utils.c
  registry.c
)

target_link_libraries(taishell
  gcc
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceModulemgrForKernel_stub
  SceModulemgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceSysrootForKernel_stub
  SceSblACMgrForDriver_stub
  SceKernelSuspendForDriver_stub
)

set_target_properties(taishell
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

vita_create_self(taishell.skprx taishell
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/taishell.yml
)

vita_create_stubs(taishell-stubs taishell ${CMAKE_SOURCE_DIR}/taishell.yml
  KERNEL
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/taishell-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)

####################
# GUI
####################

add_executable(gui
  gui/font.c
  gui/blit.c
  gui/menu.c
  gui/actions.c
  gui/gui.c
)

add_dependencies(gui taishell-stubs)

target_link_libraries(gui
  taihen_stub
  taishell_stub
  SceSysmem_stub
  SceLibKernel_stub
  SceAppmgr_stub
  SceKernelThreadmgr_stub
  SceKernelModulemgr_stub
  SceProcessmgr_stub
  SceDisplay_stub
  SceCtrl_stub
  ScePower_stub
)

set_target_properties(gui
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(gui.suprx gui
  UNSAFE
  CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui.yml
)

vita_create_stubs(taishell-gui-stubs gui ${CMAKE_SOURCE_DIR}/gui/gui.yml)

install(DIRECTORY ${CMAKE_BINARY_DIR}/taishell-gui-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)