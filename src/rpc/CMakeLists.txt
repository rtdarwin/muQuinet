# # 1. 删除先前由 protoc 生成的 C/C++ 代码

# #    方便起见，直接由 cmake execute_process 来调用 protoc 生成 C/C++ 代码
# #      而这会引起 target {rpc_c, rpc_cpp} out-of-date，
# #      从而引起 target {rpc_c, rpc_cpp} 的重编译
# #    为避免不必要的生成与重编译，
# #      protobuf 定义文件不再变动之后，comment out 下面的 1-3 步

# execute_process(
#   COMMAND cmake -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/c_out"
#   COMMAND cmake -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/cpp_out"
#   )
# execute_process(
#   COMMAND cmake -E make_directory
#       "${CMAKE_CURRENT_SOURCE_DIR}/c_out"
#       "${CMAKE_CURRENT_SOURCE_DIR}/cpp_out"
#   )

# # 2. glob protobuf 的 .proto 文件

# # file(GLOB rpc_proto_SRCS
# #   LIST_DIRECTORIES true
# #   ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto
# #   )

# set(rpc_proto_SRCS
#   ${CMAKE_CURRENT_SOURCE_DIR}/proto/request.proto
#   ${CMAKE_CURRENT_SOURCE_DIR}/proto/response.proto
#   )

# # 3. 调用 protoc 生成 C/C++ 代码

# foreach(pfile IN LISTS rpc_proto_SRCS)
#   execute_process(
#     COMMAND protoc-c
#         -I${CMAKE_CURRENT_SOURCE_DIR}/proto
#         --c_out=${CMAKE_CURRENT_SOURCE_DIR}/c_out
#         ${pfile}
#     COMMAND protoc
#         -I${CMAKE_CURRENT_SOURCE_DIR}/proto
#         --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/cpp_out
#         ${pfile}
#   )
# endforeach()

# 4. 把生成的 C/C++ 代码编译成库

file(GLOB rpc_c_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/c_out/*.c
  )
add_library(muquinet_rpc_c ${rpc_c_SRCS})
set_target_properties(muquinet_rpc_c PROPERTIES
  LINKER_LANGUAGE C
  )

file(GLOB rpc_cpp_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_out/*.cc
  )
add_library(muquinet_rpc_cpp ${rpc_cpp_SRCS})
set_target_properties(muquinet_rpc_cpp PROPERTIES
  LINKER_LANGUAGE CXX
  )
