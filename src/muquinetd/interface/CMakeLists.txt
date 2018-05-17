set(muquinetd_interface_SRCS
  Interface.cpp
  NetDev.cpp
  TunDevice.cpp
  )

add_library(muquinetd_interface
  ${muquinetd_interface_SRCS}
  )
