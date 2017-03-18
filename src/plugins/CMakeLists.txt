project(taishell-core-plugins)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/taishell-stubs
  ${CMAKE_BINARY_DIR}/taishell-gui-stubs
)

####################
# FTPD
####################

add_executable(ftpd
  ftpd/ftpd.c
  ftpd/libftpvita/ftpvita.c
  ftpd/libftpvita/sscanf.c
)

target_link_libraries(ftpd
  taihen_stub
  Taishell_stub
  SceSysmem_stub
  SceLibKernel_stub
  SceAppmgr_stub
  SceKernelThreadmgr_stub
  SceKernelModulemgr_stub
  SceProcessmgr_stub
  SceNet_stub
  SceNetCtl_stub
  SceSysmodule_stub
  SceAppUtil_stub
  SceIofilemgr_stub
)

set_target_properties(ftpd
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(ftpd.suprx ftpd
  UNSAFE
  CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/ftpd/ftpd.yml
)
