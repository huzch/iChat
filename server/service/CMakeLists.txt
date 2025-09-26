# cmake版本
cmake_minimum_required(VERSION 3.12.0)
# 工程名称
project(service)
# 添加子模块
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/speech)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/file)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/user)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/forward)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/message)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/friend)
# 设置安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})