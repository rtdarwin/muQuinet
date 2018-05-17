set(muquinetd_udp_SRCS
  Udp.cpp
  UdpPcb.cpp
  )

add_library(muquinetd_udp
  ${muquinetd_udp_SRCS}
  )
