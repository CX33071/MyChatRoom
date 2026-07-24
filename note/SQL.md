# SQL

一、分类

`DDL`:数据定义语言，用来定义数据库对象(数据库、表、字段)

`DML`：数据操作语言，用来对数据库表中的数据进行增删改

`DQL`：数据查询语言，用来查询数据库中表的记录

`DCL`：数据控制语言，用来创建数据库用户、控制数据库的访问权限

二、`DDL`

定义和管理数据库、表的结构(创建/修改/删除库和表)

关键字：`CREATE/ALTER/DROP`

**数据库操作**

1.数据库操作

​	查询所有数据库：

```mysql
SHOW DATABASES;
```

​	查询当前数据库：

```mysql
SELECT DATABASE();
```

​	使用/切换数据库

```mysql
USE 数据库名;
```

​	创建数据库：

```mysql
-- 基础语法
CREATE DATABASE [IF NOT EXISTS] 数据库名 [DEFAULT CHARSET 字符集] [COLLATE 排序规则];

-- 示例：创建名为itcast的数据库，字符集为utf8mb4
CREATE DATABASE IF NOT EXISTS itcast DEFAULT CHARSET utf8mb4;
```

​	修改/删除数据库：

```mysql
-- 1. 修改数据库字符集
ALTER DATABASE 数据库名 DEFAULT CHARSET 新字符集;

-- 2. 删除数据库
DROP DATABASE [IF EXISTS] 数据库名;
```

**表操作**

1.查询表

```mysql
-- 1. 查询当前数据库所有表
SHOW TABLES;

-- 2. 查询表结构（字段、类型、约束）
DESC 表名;

-- 3. 查询建表语句
SHOW CREATE TABLE 表名;

```

2.创建表

```mysql
-- 基础语法
CREATE TABLE 表名 (
    字段1 数据类型 [约束] [COMMENT '注释'],
    字段2 数据类型 [约束] [COMMENT '注释'],
    字段3 数据类型 [约束] [COMMENT '注释'],
    ...
) [COMMENT '表注释'];

-- 示例：创建员工表emp（和你图片中的示例一致）
CREATE TABLE emp (
    id INT COMMENT '编号',
    workno VARCHAR(10) COMMENT '工号',
    name VARCHAR(10) COMMENT '姓名',
    gender CHAR(1) COMMENT '性别',
    age TINYINT UNSIGNED COMMENT '年龄',
    idcard CHAR(18) COMMENT '身份证号',
    workaddress VARCHAR(50) COMMENT '工作地址',
    entrydate DATE COMMENT '入职时间'
) COMMENT '员工表';

```

3.修改表

```mysql
-- 1. 添加字段
ALTER TABLE 表名 ADD 字段名 数据类型 [约束] [COMMENT '注释'];

-- 2. 修改字段类型/约束
ALTER TABLE 表名 MODIFY 字段名 新数据类型 [约束];

-- 3. 修改字段名和类型
ALTER TABLE 表名 CHANGE 旧字段名 新字段名 数据类型 [约束];

-- 4. 删除字段
ALTER TABLE 表名 DROP 字段名;

-- 5. 修改表名
ALTER TABLE 表名 RENAME TO 新表名;

```

4.删除表

```mysql
-- 1. 删除表
DROP TABLE [IF EXISTS] 表名;

-- 2. 清空表（保留表结构，删除所有数据）
TRUNCATE TABLE 表名;

```

三、`DML`

对表中数据进行增、删、改操作

关键字：`INSERT/UPDATE/DELETE`

1.插入数据

```mysql
-- 语法1：给指定字段插入数据
INSERT INTO 表名 (字段1, 字段2, 字段3, ...) VALUES (值1, 值2, 值3, ...);

-- 语法2：给所有字段插入数据（需按字段顺序填写所有值）
INSERT INTO 表名 VALUES (值1, 值2, 值3, ...);

-- 语法3：批量插入数据
INSERT INTO 表名 (字段1, 字段2, ...) 
VALUES (值1, 值2, ...), (值1, 值2, ...), (值1, 值2, ...);

-- 示例：向emp表批量插入数据
INSERT INTO emp (id, workno, name, gender, age, idcard, workaddress, entrydate)
VALUES 
(1, '1', '柳岩', '女', 20, '123456789012345678', '北京', '2000-01-01'),
(2, '2', '张无忌', '男', 18, '123456789012345670', '北京', '2005-09-01'),
(3, '3', '韦一笑', '男', 38, '123456789012345679', '上海', '2005-08-01');

```

2.修改数据

```mysql
-- 语法：修改符合条件的数据（必须带WHERE条件，否则会修改所有行！）
UPDATE 表名 SET 字段1=新值1, 字段2=新值2, ... WHERE 条件;

-- 示例：修改id为1的员工年龄为21
UPDATE emp SET age=21 WHERE id=1;

-- 示例：修改所有员工的工作地址为'西安'（无WHERE条件，所有行都会被修改，慎用！）
UPDATE emp SET workaddress='西安';

```

3.删除数据

```mysql
-- 语法：删除符合条件的数据（必须带WHERE条件，否则会删除所有数据！）
DELETE FROM 表名 WHERE 条件;

-- 示例：删除id为3的员工数据
DELETE FROM emp WHERE id=3;

-- 示例：删除表中所有数据（慎用！）
DELETE FROM emp;

```

四、`DQL`

查询表中的数据，关键字：`select`

编写顺序：

```mysql
SELECT 字段列表
FROM 表名列表
WHERE 条件列表
GROUP BY 分组字段列表
HAVING 分组后过滤条件
ORDER BY 排序字段列表
LIMIT 分页参数;

```

执行顺序：FROM → WHERE → GROUP BY → HAVING → SELECT → ORDER BY → LIMIT

1. 基本查询

```mysql
-- 1. 查询指定字段
SELECT 字段1, 字段2, 字段3 FROM 表名;

-- 2. 查询所有字段（不推荐生产环境使用）
SELECT * FROM 表名;

-- 示例：查询员工的姓名、工号、年龄
SELECT name, workno, age FROM emp;
```

（2）设置别名（AS）

```mysql
-- 语法：字段名 AS 别名（AS可省略）
SELECT 字段1 [AS 别名1], 字段2 [AS 别名2] FROM 表名;

-- 示例：查询员工工作地址，别名设为“工作地址”
SELECT workaddress AS '工作地址' FROM emp;
SELECT workaddress '工作地址' FROM emp; -- AS可省略
```

（3）去除重复记录（DISTINCT）

```mysql
SELECT DISTINCT 字段列表 FROM 表名;

-- 示例：查询员工的工作地址，去除重复值
SELECT DISTINCT workaddress '工作地址' FROM emp;
```

2. 条件查询（WHERE）

```mysql
-- 语法：查询符合条件的数据
SELECT 字段列表 FROM 表名 WHERE 条件;
```

（1）常用运算符

|                运算符                |                     说明                     |
| :----------------------------------: | :------------------------------------------: |
| `>` / `<` / `>=` / `<=` / `=` / `<>` |        比较运算符（不等于也可用`!=`）        |
|        `BETWEEN ... AND ...`         |        范围查询（闭区间，包含两端值）        |
|              `IN(...)`               |      集合查询（匹配括号内的任意一个值）      |
|                `LIKE`                | 模糊查询（`%`匹配任意字符，`_`匹配单个字符） |
|      `IS NULL` / `IS NOT NULL`       |               判断是否为 NULL                |
|         `AND` / `OR` / `NOT`         |                  逻辑运算符                  |

（2）示例

```mysql
-- 1. 查询年龄等于18的员工
SELECT * FROM emp WHERE age = 18;

-- 2. 查询年龄不等于18的员工
SELECT * FROM emp WHERE age <> 18;
SELECT * FROM emp WHERE age != 18;

-- 3. 查询年龄在20~40岁之间的员工（包含20和40）
SELECT * FROM emp WHERE age BETWEEN 20 AND 40;

-- 4. 查询年龄为18、20、30的员工
SELECT * FROM emp WHERE age IN (18, 20, 30);

-- 5. 查询姓名为三个字的员工（模糊查询，_匹配单个字符）
SELECT * FROM emp WHERE name LIKE '___';

-- 6. 查询身份证号为NULL的员工
SELECT * FROM emp WHERE idcard IS NULL;

-- 7. 查询性别为男，且年龄在20~40岁之间的员工
SELECT * FROM emp WHERE gender = '男' AND age BETWEEN 20 AND 40;
```

3. 聚合函数（纵向计算）

将一列数据作为整体，进行纵向计算，`NULL`值不参与聚合函数运算。

|      函数       |     功能     |
| :-------------: | :----------: |
| `COUNT(字段/*)` | 统计记录数量 |
|   `MAX(字段)`   |   求最大值   |
|   `MIN(字段)`   |   求最小值   |
|   `AVG(字段)`   |   求平均值   |
|   `SUM(字段)`   |     求和     |

```mysql
-- 语法
SELECT 聚合函数(字段列表) FROM 表名;

-- 示例：
-- 1. 统计员工总数（COUNT(*)统计所有行，包括NULL；COUNT(字段)会忽略NULL）
SELECT COUNT(*) FROM emp;
SELECT COUNT(idcard) FROM emp;

-- 2. 统计员工平均年龄
SELECT AVG(age) FROM emp;

-- 3. 统计最大年龄
SELECT MAX(age) FROM emp;

-- 4. 统计西安地区员工的年龄之和
SELECT SUM(age) FROM emp WHERE workaddress = '西安';
```

4. 分组查询（GROUP BY）

```mysql
-- 语法
SELECT 字段列表 FROM 表名 [WHERE 条件] GROUP BY 分组字段列表 [HAVING 分组后过滤条件];
```

核心规则与 WHERE/HAVING 区别

|    对比项    |    `WHERE`     |     `HAVING`     |
| :----------: | :------------: | :--------------: |
|   执行时机   | 分组前过滤数据 | 分组后对结果过滤 |
|   过滤对象   |   原始数据行   | 分组后的统计结果 |
| 支持聚合函数 |     不支持     |       支持       |

```mysql
-- 1. 根据性别分组，统计男女员工数量
SELECT gender, COUNT(*) FROM emp GROUP BY gender;

-- 2. 根据性别分组，统计男女员工的平均年龄
SELECT gender, AVG(age) FROM emp GROUP BY gender;

-- 3. 查询年龄小于45的员工，按工作地址分组，获取员工数量≥3的工作地址
SELECT workaddress, COUNT(*) AS address_count 
FROM emp 
WHERE age < 45 
GROUP BY workaddress 
HAVING address_count >= 3;
```

5. 排序查询（ORDER BY）

```mysql
-- 语法
SELECT 字段列表 FROM 表名 ORDER BY 字段1 排序方式1, 字段2 排序方式2;
```

排序方式

- `ASC`：升序（默认值，可省略）
- `DESC`：降序

> 多字段排序时，只有第一个字段值相同，才会按第二个字段排序。

```mysql
-- 1. 按年龄升序排序
SELECT * FROM emp ORDER BY age ASC;
SELECT * FROM emp ORDER BY age; -- ASC可省略

-- 2. 按入职时间降序排序
SELECT * FROM emp ORDER BY entrydate DESC;

-- 3. 按年龄升序，年龄相同则按入职时间降序排序
SELECT * FROM emp ORDER BY age ASC, entrydate DESC;
```

6. 分页查询（LIMIT）

```mysql
-- 语法（MySQL方言，不同数据库实现不同）
SELECT 字段列表 FROM 表名 LIMIT 起始索引, 查询记录数;
```

- 起始索引从 0 开始，计算公式：`起始索引 = (页码-1) * 每页记录数`
- 查询第一页数据时，起始索引可省略，直接写`LIMIT 记录数`

```mysql
-- 1. 查询第1页数据，每页5条
SELECT * FROM emp LIMIT 0, 5;
SELECT * FROM emp LIMIT 5; -- 起始索引0可省略

-- 2. 查询第2页数据，每页5条（起始索引=(2-1)*5=5）
SELECT * FROM emp LIMIT 5, 5;

-- 3. 查询性别为男，年龄在20~40岁之间的前5个员工，按年龄升序排序
SELECT * FROM emp 
WHERE gender = '男' AND age BETWEEN 20 AND 40 
ORDER BY age ASC 
LIMIT 5;
```

五、`DCL`

管理用户，哪个用户能访问哪些数据库

1. 用户管理

（1）查询用户

```
USE mysql; -- 用户信息存在mysql数据库的user表中
SELECT * FROM user;
```

（2）创建用户

```
-- 语法
CREATE USER '用户名'@'主机名' IDENTIFIED BY '密码';

-- 示例：创建用户itcast，仅能在localhost登录，密码123456
CREATE USER 'itcast'@'localhost' IDENTIFIED BY '123456';

-- 示例：创建用户heima，可在任意主机登录（%通配符），密码123456
CREATE USER 'heima'@'%' IDENTIFIED BY '123456';
```

（3）修改用户密码

```
-- 语法（MySQL 8.0+推荐）
ALTER USER '用户名'@'主机名' IDENTIFIED WITH mysql_native_password BY '新密码';

-- 示例：修改itcast用户密码为654321
ALTER USER 'itcast'@'localhost' IDENTIFIED WITH mysql_native_password BY '654321';
```

（4）删除用户

```
-- 语法
DROP USER '用户名'@'主机名';

-- 示例：删除用户heima
DROP USER 'heima'@'%';
```

2. 权限控制

（1）常用权限说明

|          权限          |          说明          |
| :--------------------: | :--------------------: |
| `ALL / ALL PRIVILEGES` |      拥有所有权限      |
|        `SELECT`        |        查询数据        |
|        `INSERT`        |        插入数据        |
|        `UPDATE`        |        修改数据        |
|        `DELETE`        |        删除数据        |
|        `ALTER`         |       修改表结构       |
|         `DROP`         | 删除数据库 / 表 / 视图 |
|        `CREATE`        |    创建数据库 / 表     |

（2）查询用户权限

```
SHOW GRANTS FOR '用户名'@'主机名';

-- 示例：查询heima用户的权限
SHOW GRANTS FOR 'heima'@'%';
```

（3）授予用户权限（GRANT）

```
-- 语法
GRANT 权限列表 ON 数据库名.表名 TO '用户名'@'主机名';

-- 示例：给heima用户授予所有数据库的所有权限
GRANT ALL ON *.* TO 'heima'@'%';

-- 示例：给itcast用户授予itcast数据库emp表的查询权限
GRANT SELECT ON itcast.emp TO 'itcast'@'localhost';
```

（4）撤销用户权限（REVOKE）

```
-- 语法
REVOKE 权限列表 ON 数据库名.表名 FROM '用户名'@'主机名';

-- 示例：撤销heima用户在itcast数据库的所有权限
REVOKE ALL ON itcast.* FROM 'heima'@'%';
```
