
# election 
是分布式选举模块，它是比较独立的，因为在运行时如果选举不出 1 号 leader，系统所有组件就都不工作。它是独立于 Paxos 协议的。该选举协议要求各节点时钟同步。clog 最初的意思是 commitlog，现在成了专有词汇，特指 OceanBase 的事务 redo 日志。Paxos 的实现也在这个目录下。archive 是日志归档组件，备份恢复依赖这个组件。

# rootserver
目录是 OceanBase 集群总控服务。这个命名不够准确，准确的名字应该是 rootservice，它不是独立进程，而是某些 observer 内部启动的一组服务，感兴趣的读者可以看看 OceanBase 0.4 的开源代码。集群管理和自动容灾，系统自举，分区副本管理和负载均衡，以及 DDL 的执行都在这个组件中。

# share
目录是被强行从“母体”oblib/src/common 中剥离出来的公共类，所以它们的 namespace 是 common 而不是 share。

# sql
就是 SQL。storage 就是存储引擎。事务管理位于 storage/transaction 下。

# observer
是所有组件的“总装车间”，入口是 ob_server.h 和 ob_service.h。MySQL 协议层的命令处理入口位于 observer/mysql。