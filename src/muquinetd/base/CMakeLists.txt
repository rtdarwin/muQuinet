set(muquinetd_base_SRCS
  CmdRunner.cpp
  InternetChecksum.cpp
  MutexLock.cpp
  )

add_library(muquinetd_base
  ${muquinetd_base_SRCS}
  )
