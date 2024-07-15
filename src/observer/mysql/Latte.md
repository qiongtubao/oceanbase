


# mysql

除了建立和断开连接，MySQL 协议大多是简单的请求响应模型。每种请求类似一个 COM_XXX 命令，每种命令的处理函数对应在本目录有一个文件（类）。比如最常用的 COM _QUERY 表示一条 SQL 请求，处理类就位于 obmp_query.h/cpp 中。一般典型的交互过程是 connect、query、query ... query、quit。注意，所有的 SQL 语句类型，包括 DML、DDL，以及 multi-statement 都是用 query 命令处理的。

建立连接的过程在 obmp_connect，它执行用户认证鉴权，如果鉴权成功，会创建一个 ObSQLSession 对象（位于 src/sql/session）唯一表示一个数据库连接。所有其他命令处理都会访问这个 session 对象。


## obmp_connect

建立连接的过程在 obmp_connect，它执行用户认证鉴权，如果鉴权成功，会创建一个 ObSQLSession 对象（位于 src/sql/session）唯一表示一个数据库连接。所有其他命令处理都会访问这个 session 对象。


## 命令解析

继承ObMPBase 类似与OBMPQuery 入口proces