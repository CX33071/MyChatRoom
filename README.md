# `MyChatRoom`

基于 C++17 开发的 Linux 命令行即时通讯系统。 本项目采用 C/S（Client/Server）架构，通过 TCP 长连接实现客户端与服务器之间的实时通信。 项目主要用于学习 Linux 网络编程、多线程并发、数据库存储以及服务器架构设计。

支持用户管理、好友聊天、群聊、文件传输、断点续传以及聊天记录保存等功能。


# 功能介绍

## 用户系统

- 用户注册
- 用户登录
- 验证码登录
- 修改密码
- 注销账号


## 好友系统

- 添加好友
- 好友请求处理
- 好友列表
- 删除好友
- 拉黑好友
- 好友在线聊天
- 历史聊天记录保存
- 好友上传、下载文件、断点续传
- 离线消息保存


## 群聊系统

- 创建群聊
- 申请加入群聊
- 群主或管理员处理加群申请
- 群主或管理员移除群成员
- 群主或管理员设置群管理员
- 转移群主
- 在线群聊天
- 保存历史聊天记录
- 群聊上传文件、下载文件、断点续传
- 退出群聊
- 解散群聊


## 文件系统

- 文件上传
- 文件下载
- 上传/下载进度显示
- 断点续传


## 数据存储

客户端：

- `SQLite`保存本地聊天记录


服务器：

- `MySQL`保存用户、好友、文件等数据
- `Redis`缓存登录状态、验证码等信息

# 环境要求

推荐环境：`Linux`操作系统(`Ubuntu`等)、`C++17`、`CMake`

依赖：

```c++
mysql
redis
sqlite3
readline
curl
nlohmann/json
pthread
```

```c++
# 安装依赖
sudo apt update
sudo apt install cmake
```

安装数据库

```c
sudo apt install mysql-server
sudo apt install redis-server
sudo apt install sqlite3 
```

安装其他依赖

```c
sudo apt install libcurl4-openssl-dev
sudo apt install libreadline-dev
```

数据库配置

## `MySQL`

创建数据库：

```
CREATE DATABASE MyChatRoom;
```

修改服务器 `MySQL` 配置：

填写：

```c
host: 127.0.0.1
user: root
password: 数据库密码
database: MyChatRoom
```

## `Redis`

启动：

```c++
redis-server
```

测试：

```c++
redis-cli ping
```

返回：

```c++
PONG
```

表示成功。

# 编译

进入项目目录：

```c++
mkdir build
cd build
cmake ..
make
```

编译完成生成：

```c++
server
client
```

------

# 运行

## 启动服务器

```c++
./server 0.0.0.0 8888 9999
```

## 启动客户端

```c++
./client 服务端IP
```

本机测试：

```c++
./client 127.0.0.1
```

局域网测试：

```c++
./client 服务器局域网IP
```