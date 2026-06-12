cmake_minimum_required(VERSION 3.10)
project(MyChatRoom)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# ===== 包含目录 =====
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net)
include_directories(/usr/local/include)

# ===== 查找必要的库 =====
find_package(CURL REQUIRED)
find_library(CPP_REDIS_LIB NAMES cpp_redis libcpp_redis PATHS /usr/local/lib /usr/lib)
find_library(TACOPIE_LIB NAMES tacopie libtacopie PATHS /usr/local/lib /usr/lib)

# ===== muduo源文件 =====
set(MUDUO_BASE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/base/CurrentThread.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/base/logger.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/base/logStream.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/base/Timestamp.cc
)

set(MUDUO_NET_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Acceptor.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Channel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Connector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Epoller.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/EventLoop.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/EventLoopThreadpool.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/SocketOps.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/TcpClient.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/TcpConnection.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/TcpServer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/Timer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../muduo-/net/TimerQueue.cc
)

# ===== 聊天室源文件（不包含 protobuf） =====
set(CHATROOM_COMMON_SOURCES
    account.cc
    friend.cc
    group.cc
)

# ===== 服务器 =====
add_executable(server 
    server.cc 
    ${CHATROOM_COMMON_SOURCES}
    ${MUDUO_BASE_SOURCES}
    ${MUDUO_NET_SOURCES}
)

# ===== 客户端 =====
add_executable(client 
    client.cc 
    ${CHATROOM_COMMON_SOURCES}
    ${MUDUO_BASE_SOURCES}
    ${MUDUO_NET_SOURCES}
)

# ===== 链接库 =====
target_link_libraries(server pthread)
target_link_libraries(client pthread)

# curl 库
target_link_libraries(server ${CURL_LIBRARIES})
target_link_libraries(client ${CURL_LIBRARIES})

# cpp_redis 和 tacopie
if(CPP_REDIS_LIB AND TACOPIE_LIB)
    target_link_libraries(server ${CPP_REDIS_LIB} ${TACOPIE_LIB})
    target_link_libraries(client ${CPP_REDIS_LIB} ${TACOPIE_LIB})
else()
    target_link_libraries(server /usr/local/lib/libcpp_redis.so /usr/local/lib/libtacopie.a)
    target_link_libraries(client /usr/local/lib/libcpp_redis.so /usr/local/lib/libtacopie.a)
endif()

# 添加 RPATH
set_target_properties(server PROPERTIES 
    INSTALL_RPATH "/usr/local/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
set_target_properties(client PROPERTIES 
    INSTALL_RPATH "/usr/local/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)