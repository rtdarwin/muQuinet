set(muquinetd_mux_SRCS
  Mux.cpp
  RequestHandler.cpp
  Socket.cpp

  eventloop/SelectableChannel.cpp
  eventloop/EventLoop.cpp
  eventloop/Poller.cpp

  req-resp-channel/MasterListener.cpp
  req-resp-channel/ReqRespChannel.cpp
  )

add_library(muquinetd_mux
  ${muquinetd_mux_SRCS}
  )
