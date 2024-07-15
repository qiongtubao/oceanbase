
# 快照
OBITsMgr



# 事务
不论单机还是多机场景，只要是多分区事务 OceanBase 均称为分布式事务。
为了性能的考虑，单机多分区事务和单分区事务的事务提交，做了一定的优化。
单机多分区事务提交，本质上依然是两阶段，而单分区事务完全走一阶段提交。

ObScheTransCtx::end_trans(xxx) =>
ObTransCoordCtx::hande_message(xxx) =>
ObPartTransCtx::handle_message(xxx);


