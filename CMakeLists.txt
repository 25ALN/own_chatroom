cmake_minimum_required(VERSION 2.8)
project(testwjj)

set(CMAKE_BUILD_TYPE Release)

# 找 hiredis
find_package(PkgConfig REQUIRED)
pkg_search_module(HIREDIS REQUIRED hiredis)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${HIREDIS_INCLUDE_DIRS}
)

# 自动收集源码
file(GLOB SERVER_SRC
    ${PROJECT_SOURCE_DIR}/src/server/*.cpp
    ${PROJECT_SOURCE_DIR}/src/common/*.cpp
)

file(GLOB CLIENT_SRC
    ${PROJECT_SOURCE_DIR}/src/client/*.cpp
    ${PROJECT_SOURCE_DIR}/src/ccommon/*.cpp
)

# 生成可执行文件
add_executable(chat_server ${SERVER_SRC})
add_executable(chat_client ${CLIENT_SRC})

# 链接库
find_package(Threads REQUIRED)
target_link_libraries(chat_server PRIVATE Threads::Threads ${HIREDIS_LIBRARIES})
target_link_libraries(chat_client PRIVATE Threads::Threads)
