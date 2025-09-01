# cmake版本
cmake_minimum_required(VERSION 3.12.0)
# 工程名称
project(speech_server)
# 目标名称
set(target "speech_server")

set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../../proto)
set(proto_files speech.proto)
set(proto_hh "")
set(proto_cc "")
set(proto_src "")

foreach(proto_file ${proto_files})
  string(REPLACE ".proto" ".pb.h" proto_hh ${proto_file})
  string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})

  set(proto_hh_path ${CMAKE_CURRENT_BINARY_DIR}/${proto_hh})
  set(proto_cc_path ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})

  if(NOT EXISTS ${proto_hh_path} OR NOT EXISTS ${proto_cc_path})
    add_custom_command(
      COMMAND protoc
      ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} ${proto_path}/${proto_file}
      DEPENDS ${proto_path}/${proto_file}
      OUTPUT ${proto_hh_path} ${proto_cc_path}
      COMMENT "生成protobuf框架代码:  ${proto_hh_path} and ${proto_cc_path}"
    )
  endif()
  # 收集proto生成的源文件
  list(APPEND proto_src ${proto_cc_path})
endforeach()

# 收集自己编写的源文件
set(src "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src)
# 添加执行目标和依赖
add_executable(${target} ${proto_src} ${src})
# 添加动态链接库
target_link_libraries(${target} -lgflags -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp)
# 头文件搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third/include)