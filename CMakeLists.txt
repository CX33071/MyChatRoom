cmake_minimum_required(VERSION 3.10)

project(MyChatRoom)

# =========================================================
# C++
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -g -Wall -pthread")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# =========================================================
# Include
# =========================================================

include_directories(

    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/client
    ${PROJECT_SOURCE_DIR}/server

    ${PROJECT_SOURCE_DIR}/../muduo-/base
    ${PROJECT_SOURCE_DIR}/../muduo-/net

    /usr/local/include
)

# =========================================================
# 查找库
# =========================================================

find_package(CURL REQUIRED)

# -------------------------
# MySQL C API
# -------------------------

find_path(MYSQL_INCLUDE_DIR
    NAMES mysql/mysql.h
    PATHS
        /usr/include
        /usr/local/include
)

find_library(MYSQL_CLIENT_LIB
    NAMES mysqlclient
    PATHS
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)

if(NOT MYSQL_CLIENT_LIB)
    message(FATAL_ERROR "mysqlclient library not found!")
endif()

include_directories(${MYSQL_INCLUDE_DIR})

# -------------------------
# Redis
# -------------------------

find_library(CPP_REDIS_LIB
    NAMES cpp_redis libcpp_redis
    PATHS
        /usr/local/lib
        /usr/lib
)

find_library(TACOPIE_LIB
    NAMES tacopie libtacopie
    PATHS
        /usr/local/lib
        /usr/lib
)

# =========================================================
# muduo
# =========================================================

set(MUDUO_BASE

    ../muduo-/base/CurrentThread.cc
    ../muduo-/base/logger.cc
    ../muduo-/base/logStream.cc
    ../muduo-/base/Timestamp.cc
)

set(MUDUO_NET

    ../muduo-/net/Acceptor.cc
    ../muduo-/net/Buffer.cc
    ../muduo-/net/Channel.cc
    ../muduo-/net/Connector.cc
    ../muduo-/net/Epoller.cc
    ../muduo-/net/EventLoop.cc
    ../muduo-/net/EventLoopThreadpool.cc
    ../muduo-/net/SocketOps.cc
    ../muduo-/net/TcpClient.cc
    ../muduo-/net/TcpConnection.cc
    ../muduo-/net/TcpServer.cc
    ../muduo-/net/Timer.cc
    ../muduo-/net/TimerQueue.cc
)

# =========================================================
# Server
# =========================================================

set(SERVER_SRC

    server/server.cc

    server/account.cc
    server/friend.cc
    server/group.cc

    server/Mysql.cc
    server/ChatServer.cc
    server/FileServer.cc
    server/FILEredis.cc

    ${MUDUO_BASE}
    ${MUDUO_NET}
)

add_executable(server ${SERVER_SRC})
find_package(glog REQUIRED)


target_link_libraries(
    server
    glog::glog
)
# =========================================================
# Client
# =========================================================

set(CLIENT_SRC

    client/client.cc

    client/ChatClient.cc
    client/FileClient.cc

    ${MUDUO_BASE}
    ${MUDUO_NET}
)

add_executable(client ${CLIENT_SRC})

# =========================================================
# Server 链接库
# =========================================================

target_link_libraries(server

    pthread

    ${CURL_LIBRARIES}

    ${MYSQL_CLIENT_LIB}
)

# =========================================================
# Client 链接库
# =========================================================

target_link_libraries(client

    pthread

    ${CURL_LIBRARIES}
)

# =========================================================
# Redis
# =========================================================

if(CPP_REDIS_LIB AND TACOPIE_LIB)

    target_link_libraries(server

        ${CPP_REDIS_LIB}
        ${TACOPIE_LIB}
    )

    target_link_libraries(client

        ${CPP_REDIS_LIB}
        ${TACOPIE_LIB}
    )

else()

    target_link_libraries(server

        /usr/local/lib/libcpp_redis.so
        /usr/local/lib/libtacopie.a
    )

    target_link_libraries(client

        /usr/local/lib/libcpp_redis.so
        /usr/local/lib/libtacopie.a
    )

endif()

# =========================================================
# RPATH
# =========================================================

set_target_properties(server PROPERTIES

    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/usr/local/lib"
)

set_target_properties(client PROPERTIES

    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/usr/local/lib"
)