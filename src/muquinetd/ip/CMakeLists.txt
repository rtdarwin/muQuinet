set(muquinetd_ip_SRCS
  Ip.cpp
  IpHeader.cpp
  frag/Defrager.cpp
  frag/Frager.cpp
  )

add_library(muquinetd_ip
  ${muquinetd_ip_SRCS}
  )
