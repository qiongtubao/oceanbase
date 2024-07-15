

# ob_partition_service.h

storage/ob_partition_service.h 是存储层的总入口，它对外提供了存储层的所有 RPC 服务，如创建删除分区副本。实际上，它是每个节点上所有分区级接口的入口，包括事务控制接口、分区读写的接口等。


# RS
前面讲过，建表和新增分区等 DDL 语句是由 RS 执行的
RS 在根据一定策略选定节点后，就会 RPC 调用这里的 create_xxx。


# OBPartitionStore