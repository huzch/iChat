# cmake版本
cmake_minimum_required(VERSION 3.12.0)
# 工程名称
project(user_service)
# 目标名称
set(src_target "user_server")
set(test_target "user_client")

set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../../proto)
set(proto_files base.proto file.proto user.proto)
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
      ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
      DEPENDS ${proto_path}/${proto_file}
      OUTPUT ${proto_hh_path} ${proto_cc_path}
      COMMENT "生成protobuf框架代码:  ${proto_hh_path} and ${proto_cc_path}"
    )
  endif()
  # 收集proto生成的源文件
  list(APPEND proto_src ${proto_cc_path})
endforeach()

set(odb_path ${CMAKE_CURRENT_SOURCE_DIR}/../../odb)
set(odb_files user.hxx)
set(odb_hxx "")
set(odb_cxx "")
set(odb_src "")

foreach(odb_file ${odb_files})
  string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
  string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})

  set(odb_hxx_path ${CMAKE_CURRENT_BINARY_DIR}/${odb_hxx})
  set(odb_cxx_path ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})

  if(NOT EXISTS ${odb_hxx_path} OR NOT EXISTS ${odb_cxx_path})
      add_custom_command(
        PRE_BUILD
        COMMAND odb
        ARGS -d mysql --std c++11 --generate-query --generate-schema ${odb_path}/${odb_file}
        DEPENDS ${odb_path}/${odb_file}
        OUTPUT ${odb_hxx_path} ${odb_cxx_path}
        COMMENT "生成odb框架代码:  ${odb_hxx_path} and ${odb_cxx_path}"
      )
  endif()
  # 收集odb生成的源文件
  list(APPEND odb_src ${odb_cxx_path})
endforeach()

# 收集自己编写的源文件
set(src "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src)
set(test_src "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_src)
# 添加执行目标和依赖
add_executable(${src_target} ${proto_src} ${odb_src} ${src})
add_executable(${test_target} ${proto_src} ${test_src})
# 头文件搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third/include)
include_directories(/usr/local/include)
# 添加动态链接库
target_link_directories(${src_target} PRIVATE /usr/local/lib)
target_link_libraries(${src_target}
  -lgflags
  -lspdlog
  -lfmt
  -lbrpc
  -lssl
  -lcrypto
  -lprotobuf
  -lleveldb
  -letcd-cpp-api
  -lcpprest
  -lcurl
  -ljsoncpp
  -lodb
  -lodb-mysql
  -lodb-boost
  -lelasticlient
  -lhiredis
  -lredis++
  -lcpr
)
target_link_directories(${test_target} PRIVATE /usr/local/lib)
target_link_libraries(${test_target}
  -lgflags
  -lgtest
  -lspdlog
  -lfmt
  -lbrpc
  -lssl
  -lcrypto
  -lprotobuf
  -lleveldb
  -letcd-cpp-api
  -lcpprest
  -lcurl
  -ljsoncpp
)
# 安装路径
INSTALL(TARGETS ${src_target} ${test_target} RUNTIME DESTINATION bin)