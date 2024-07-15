
# omt
中的 mt 表示 multi-tenant，里面实现了 observer 线程模型的抽象 worker，每个租户在其有租户的节点上会创建一个线程池用于处理 SQL 请求。


# mysql
目录就是 MySQL 协议层，实现了 MySQL 5. 6兼容的消息处理协议。

# virtual_table
目录下是 sys 租户各个 __all_virtual 虚拟表的实现，“虚拟表”是 OceanBase 土话（其实是 view），它把一些内存数据结构抽象成表接口暴露出来，用于诊断调试等