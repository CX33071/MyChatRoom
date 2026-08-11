cmake_minimum_required(VERSION 3.10)

project(MyChatRoom)

# =========================================================
# C++ 设置
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -g -Wall -pthread"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}
)

# 如果需要 AddressSanitizer，可以取消下面注释
#
# set(CMAKE_CXX_FLAGS
#     "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O0"
# )
#
# set(CMAKE_EXE_LINKER_FLAGS
#     "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address"
# )


# =========================================================
# 项目路径
# =========================================================

set(PROJECT_DIR
    ${PROJECT_SOURCE_DIR}
)

set(MUDUO_DIR
    ${PROJECT_SOURCE_DIR}/../muduo-
)


# =========================================================
# Include
# =========================================================

include_directories(
    ${PROJECT_DIR}
    ${PROJECT_DIR}/client
    ${PROJECT_DIR}/server

    # Muduo
    ${MUDUO_DIR}/base
    ${MUDUO_DIR}/net

    # 第三方库
    /usr/local/include
)


# =========================================================
# 查找第三方库
# =========================================================

# -------------------------
# CURL
# -------------------------

find_package(CURL REQUIRED)


# -------------------------
# SQLite
# -------------------------

find_package(SQLite3 REQUIRED)


# -------------------------
# glog
# -------------------------

find_package(glog REQUIRED)


# -------------------------
# readline
# -------------------------

find_library(
    READLINE_LIB
    readline
    PATHS
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)

if(NOT READLINE_LIB)
    message(FATAL_ERROR "readline library not found!")
endif()


# =========================================================
# MySQL C API
# =========================================================

find_path(
    MYSQL_INCLUDE_DIR
    NAMES mysql/mysql.h
    PATHS
        /usr/include
        /usr/local/include
)

if(NOT MYSQL_INCLUDE_DIR)
    message(FATAL_ERROR "mysql/mysql.h not found!")
endif()


find_library(
    MYSQL_CLIENT_LIB
    NAMES mysqlclient
    PATHS
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)

if(NOT MYSQL_CLIENT_LIB)
    message(FATAL_ERROR "mysqlclient library not found!")
endif()

include_directories(
    ${MYSQL_INCLUDE_DIR}
)


# =========================================================
# Redis
# cpp_redis + tacopie
# =========================================================

find_library(
    CPP_REDIS_LIB
    NAMES cpp_redis libcpp_redis
    PATHS
        /usr/local/lib
        /usr/lib
)

find_library(
    TACOPIE_LIB
    NAMES tacopie libtacopie
    PATHS
        /usr/local/lib
        /usr/lib
)

if(NOT CPP_REDIS_LIB)
    message(FATAL_ERROR "cpp_redis library not found!")
endif()

if(NOT TACOPIE_LIB)
    message(FATAL_ERROR "tacopie library not found!")
endif()


# =========================================================
# Muduo Base
# =========================================================

set(MUDUO_BASE
    ${MUDUO_DIR}/base/CurrentThread.cc
    ${MUDUO_DIR}/base/logger.cc
    ${MUDUO_DIR}/base/logStream.cc
    ${MUDUO_DIR}/base/Timestamp.cc
)


# =========================================================
# Muduo Net
# =========================================================

set(MUDUO_NET
    ${MUDUO_DIR}/net/Acceptor.cc
    ${MUDUO_DIR}/net/Buffer.cc
    ${MUDUO_DIR}/net/Channel.cc
    ${MUDUO_DIR}/net/Connector.cc
    ${MUDUO_DIR}/net/Epoller.cc
    ${MUDUO_DIR}/net/EventLoop.cc
    ${MUDUO_DIR}/net/EventLoopThreadpool.cc
    ${MUDUO_DIR}/net/SocketOps.cc
    ${MUDUO_DIR}/net/TcpClient.cc
    ${MUDUO_DIR}/net/TcpConnection.cc
    ${MUDUO_DIR}/net/TcpServer.cc
    ${MUDUO_DIR}/net/Timer.cc
    ${MUDUO_DIR}/net/TimerQueue.cc
)


# =========================================================
# Server
# =========================================================

set(SERVER_SRC

    # Server main
    server/server.cc

    # ChatServer
    server/ChatServer/ChatServer.cc

    # FileServer
    server/FileServer/FileServer.cc

    # 业务层
    server/Service/account.cc
    server/Service/friend.cc
    server/Service/group.cc
    server/Service/FILEredis.cc

    # 数据库
    server/database/Mysql.cc

    # Muduo
    ${MUDUO_BASE}
    ${MUDUO_NET}
)


add_executable(
    server
    ${SERVER_SRC}
)


# =========================================================
# Client
# =========================================================

set(CLIENT_SRC

    # Client main
    client/client.cc

    # ChatClient
    client/ChatClient/ChatClient.cc

    # FileClient
    client/FileClient/FileClient.cc

    # SQLite
    client/database/SQlite.cc

    # Muduo
    ${MUDUO_BASE}
    ${MUDUO_NET}
)


add_executable(
    client
    ${CLIENT_SRC}
)


# =========================================================
# Server 链接库
# =========================================================

target_link_libraries(
    server

    pthread

    # CURL
    ${CURL_LIBRARIES}

    # MySQL
    ${MYSQL_CLIENT_LIB}

    # SQLite
    SQLite::SQLite3

    # Glog
    glog::glog

    # Redis
    ${CPP_REDIS_LIB}
    ${TACOPIE_LIB}
)


# =========================================================
# Client 链接库
# =========================================================

target_link_libraries(
    client

    pthread

    # CURL
    ${CURL_LIBRARIES}

    # SQLite
    SQLite::SQLite3

    # readline
    ${READLINE_LIB}

    # Redis
    ${CPP_REDIS_LIB}
    ${TACOPIE_LIB}
)


# =========================================================
# RPATH
# =========================================================

set_target_properties(
    server
    PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/usr/local/lib"
)


set_target_properties(
    client
    PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/usr/local/lib"
)


# =========================================================
# 输出构建信息
# =========================================================

message(STATUS "========================================")
message(STATUS "Project      : ${PROJECT_NAME}")
message(STATUS "C++ Standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Muduo        : ${MUDUO_DIR}")
message(STATUS "MySQL        : ${MYSQL_CLIENT_LIB}")
message(STATUS "Redis        : ${CPP_REDIS_LIB}")
message(STATUS "Tacopie      : ${TACOPIE_LIB}")
message(STATUS "readline     : ${READLINE_LIB}")
message(STATUS "========================================")