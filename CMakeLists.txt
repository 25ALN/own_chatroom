cmake_minimum_required(VERSION 2.8)
project(chatroom)

SET(CMAKE_BUILD_TYPE Release)

find_package(PkgConfig REQUIRED)  
pkg_search_module(HIREDIS REQUIRED hiredis)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(
${PROJECT_SOURCE_DIR}/include
${HIREDIS_INCLUDE_DIRS}
)

add_executable(chat_server src/server/server.cpp)
add_executable(chat_client src/client/client.cpp)

find_package(Threads REQUIRED)

target_link_libraries(chat_server PRIVATE Threads::Threads
${HIREDIS_LIBRARIES})
target_link_libraries(chat_client PRIVATE Threads::Threads)

