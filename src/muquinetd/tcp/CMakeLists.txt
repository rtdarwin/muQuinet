set(muquinetd_tcp_SRCS
  Tcp.cpp
  TcpPcb.cpp
  )

add_library(muquinetd_tcp
  ${muquinetd_tcp_SRCS}
  )
