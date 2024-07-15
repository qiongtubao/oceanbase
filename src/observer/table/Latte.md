


# Table Api

上面文件中，叫做 xxx_processor 的，主要都是协议层的处理。

execute_processor 是单行操作的处理函数，
batch_execute_processor 是批量多行操作的处理函数，
query_processor 是扫描操作的处理函数。

主体的逻辑在 ob_table_service.h/cpp中。
通过学习 Table API 的代码，
可以快速学习 OceanBase 存储层、事务层对 SQL 层提供的接口。

Table API 的 rpc 请求和结果的数据结构，定义在 src/share/table 中。从 ObTableOperation 类的定义中，
可以看到 Table API 提供了单行的 
insert, delete, insert or update(无索引时即 put ), replace, retrieve(get), 
increment, append，
以及他们任意组合的批量操作。
通过 ObTableQuery 接口，提供了范围扫描操作。