一、`redis`基础连接命令

```mysql
# 本地默认连接（127.0.0.1:6379）
redis-cli

# 指定IP、端口、密码连接
redis-cli -h IP地址 -p 端口 -a 密码

```

二、基础操作

```mysql
PING                # 心跳检测，正常返回 PONG
SET k1 v1           # 设置键值
GET k1              # 获取值
SELECT 数据库编号   # 切换数据库（Redis 默认 0~15 共16个库）

```

三、通用命令，针对`key`操作，不区分`value`类型

| 命令               | 作用                       | 示例 & 说明                                       |
| ------------------ | -------------------------- | ------------------------------------------------- |
| `KEYS pattern`     | 匹配查询所有符合规则的 key | `KEYS *` 查看全部 key⚠️ 生产环境禁用，会阻塞服务   |
| `DEL key [key...]` | 删除一个 / 多个 key        | `DEL k1 k2`                                       |
| `EXISTS key`       | 判断 key 是否存在          | 存在返回 1，不存在返回 0                          |
| `EXPIRE key 秒数`  | 给 key 设置过期时间        | `EXPIRE code 300` 5 分钟后自动删除                |
| `TTL key`          | 查看 key 剩余过期时间      | 正数：剩余秒数-1：永久有效-2：key 已失效 / 不存在 |

四、`redis`五大基本数据类型及命令

1.`string`字符串类型

最基础类型，数字类型支持自增、自减

```mysql
# 1. 增、改、查
SET key value                 # 设置键值（存在则覆盖）
GET key                       # 获取值

# 2. 批量操作
MSET k1 v1 k2 v2 k3 v3        # 批量设置
MGET k1 k2 k3                 # 批量获取

# 3. 过期 + 不存在才设置（常用做缓存、分布式锁）
SETEX key 秒数 value          # 设置值 + 同时指定过期时间
SETNX key value               # key 不存在才设置，存在则不操作

# 4. 数字自增/自减
INCR key                      # 整数 +1
INCRBY key 步长               # 整数 + 指定数值
INCRBYFLOAT key 小数步长      # 浮点数累加

```

`key`命名规范：

格式：项目名：业务：标识：`id`

`eg : heima:user:1、heima:goods:1001`

对象建议存`JSON`字符串

2.`Hash`哈希类型

`value`是字段-值结构，类似`map`

可以单独修改某一个字段，适合存储对象

```mysql
# 单个字段操作
HSET key field value                # 设置哈希字段
HGET key field                      # 获取单个字段值

# 批量字段操作
HMSET key f1 v1 f2 v2               # 批量设置字段
HMGET key f1 f2                     # 批量获取字段

# 全量获取
HGETALL key                         # 获取所有 field 和 value
HKEYS key                           # 只获取所有字段名
HVALS key                           # 只获取所有字段值

# 其他
HINCRBY key field 步长              # 哈希内数字字段自增
HSETNX key field value              # 字段不存在才设置

```

3.`list`列表类型

底层双向链表，元素有序、可重复

头尾增删快，中间查询慢

适用：消息队列、评论列表、时序数据

```mysql
# 左右两侧添加元素
LPUSH key 元素1 元素2...    # 从 左侧(头部) 插入
RPUSH key 元素1 元素2...    # 从 右侧(尾部) 插入

# 左右两侧弹出元素（取出并删除）
LPOP key                    # 左侧弹出
RPOP key                    # 右侧弹出

# 范围查询（下标从0开始，-1代表最后一个元素）
LRANGE key 起始下标 结束下标

# 阻塞式弹出（无元素则等待，实现阻塞队列）
BLPOP key 超时秒数
BRPOP key 超时秒数

```

业务模型

栈:`LPUSH+LPOP`

队列:`RPUSH+LPOP`

阻塞队列:`LPUSH+BRPOP`

4.`set`集合类型

无序、元素不可重复，底层哈希表

支持集合运算

适用：好友、点赞、去重、共同关注

```mysql
SADD key 元素...            # 添加元素（自动去重）
SREM key 元素...            # 删除指定元素
SMEMBERS key                # 查看集合所有元素
SCARD key                   # 获取集合元素个数
SISMEMBER key 元素          # 判断元素是否在集合中

# 集合运算
SINTER k1 k2                # 交集（共同元素）
SUNION k1 k2                # 并集（合并去重）
SDIFF k1 k2                 # 差集（k1独有元素）

```

5.`sorted set`

有序、元素不重复

每个元素包含`value+score`，按分数自动排序

适用：排行榜、积分排名、延迟队列

```mysql
ZADD key 分数1 值1 分数2 值2 ...        # 添加元素+指定分数
ZRANGE key 起始下标 结束下标            # 按分数升序查询
ZREVRANGE key 起始下标 结束下标        # 按分数降序查询

ZREM key 值                            # 删除元素
ZCARD key                              # 统计元素总数
ZSCORE key 值                          # 查询元素对应分数

```

五、五大类型总结 & 场景选型

1. **String**：普通缓存、计数器、验证码、简单键值、JSON 对象
2. **Hash**：存储结构化对象（用户、商品信息），局部更新字段
3. **List**：有序列表、评论、动态、消息队列
4. **Set**：去重、好友 / 粉丝、交集对比
5. **ZSet**：各类排行榜、积分排名、有序延时任务

六、补充易错点

1. `KEYS *` 禁止线上使用，数据量大时阻塞 Redis
2. `TTL` 返回值：-1 永久有效，-2 key 不存在
3. List 是链表，头尾操作快，中间遍历慢
4. Set / ZSet 元素都不可重复；List 允许重复
5. Hash 相比 String 存对象，优势是**单独更新字段**

七、`cpp_redis::client`核心函数大全

1.连接

```c++
redis.connect("127.0.0.1",6379);
redis.disconnect();
bool ok=redis.is_connected();
```

2.提交命令

```c
//同步提交，所有命令都会在这里真正发给reids
redis.sync_commit();
```

3.字符串操作

```c
redis.set("name","zhangfei");
redis.get("name",callback);
redis.del({"name"});
redis.exists({"name"});
redis.expire("name",60);
redis.incr("count");
```

4.哈希操作

```c
redis.hset("user:1","name","list");
redis.hget("user:1","name");
redis.hgetall("user:1");
redis.hdel("user:1","name");
redis.hexists("user:1","name");
```

5.列表操作

```c
redis.lpush("list","a");
redis.rpush("list","b");
redis.lpop("list");
redis.rpop("list");
redis.lrange("list",0,-1);
```

6.集合操作

```c
redis.sadd("set_key","v1");
redis.smembers("set_key");
redis.srem("set_key","v1");
redis.scard("set_key");
redis.sismember("set_key","v1");
```

7.有序集合

```c
redis.zadd("zset",100,"math");
redis.zrange("zset",0,-1);
redis.zrem("zset","math");
```

8.回调函数写法

```c
redis.get("name",[](cpp_redis::reply& r){
if(r.is_string()){
cout<<r.as_string()<<endl;
}
});
```

`redis`是网络操作，不会立刻拿结果，必须等`redis`服务器恢复之后自动通过回调函数把结果给你

9.获取`value`

`cpp_redis::reply`是`cpp_redis`返回结果的类型，专门存`cpp_redis`返回的数据

```c
bool checkCode(const string& email, const string& inputCode) {
   std::string s1;
    // ========= 固定 get 模板 =========
    m_redis.get(key, [&](cpp_redis::reply& reply) {
        if (reply.is_string()) {
            s1 = reply.as_string();
        }
    });
//redis.get()返回的是cpp_redis自己的futrue类型，拿到reply结果的包装器
    // 必须提交！
    m_redis.sync_commit();

}

```

