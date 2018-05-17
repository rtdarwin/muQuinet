set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${muQuinet_RUNTIME_OUTPUT_DIR}/test")

add_executable(interceptor_conn_udp_test
  interceptor.conn-udp-test.c)

add_executable(interceptor_unconn_udp_test
  interceptor.unconn-udp-test.c)

add_executable(interceptor_tcp_test
  interceptor.tcp-test.c)

add_executable(interceptor_multisocket_test
  interceptor.multisocket-test.c)

add_executable(interceptor_fork_test
  interceptor.fork-test.c)

add_executable(interceptor_pthread_test
  interceptor.pthread-test.c)
