set(muQuinetd_SRCS
  ConfReader.cpp
  Logging.cpp
  main.cpp
  muQuinetd.cpp
  Pcb.cpp
  )

add_executable(muQuinetd "${muQuinetd_SRCS}")
set_target_properties(muQuinetd
  PROPERTIES  OUTPUT_NAME muquinetd
  )

add_subdirectory(base)
add_subdirectory(interface)
add_subdirectory(ip)
# add_subdirectory(icmp)
add_subdirectory(tcp)
add_subdirectory(udp)
add_subdirectory(mux)
target_link_libraries(muQuinetd
  muquinetd_mux
  muquinetd_tcp
  muquinetd_udp
  # muquinetd_icmp
  muquinetd_ip
  muquinetd_interface
  muquinetd_base
  muquinet_rpc_cpp
  Boost::program_options
  Boost::log
  protobuf::libprotobuf
  ev
  )
