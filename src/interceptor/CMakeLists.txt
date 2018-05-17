set(muQuinet_interceptor_SRCS
  interceptor.c
  fd2channel.c
  fd-assigner.c
  req-resp-channel.c
  )

add_library(muquinet_interceptor SHARED
  ${muQuinet_interceptor_SRCS})

target_link_libraries(muquinet_interceptor
  muquinet_rpc_c
  protobuf-c
)
