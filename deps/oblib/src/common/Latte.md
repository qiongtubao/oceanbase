

# object
目录下是最重要的数据类型 ObObj 的定义，OceanBase 支持的列数据类型，这从枚举类型 ObObjType 中可以看出来。可以看出 36 以后是 Oracle 租户类型下的数据类型。ObObj 是存储和数据处理的“原子”


# rowkey
目录下定义的 ObRowkey 是每一行记录的主键。OceanBase 在底层存储只有索引组织表，每一行必须有主键；用户可见的无主键表是通过一个隐藏的自增列做 rowkey的，算是一个模拟。存储引擎的 memtable 和 sstable 中都是用 rowkey 索引的

# row
 目录下定义了一行记录的表示 ObNewRow （你找不到ObRow:），他是数据处理的“分子”，基于它定义的 ObRowIterator 是很多操作类的接口。

# log
目录定义了一组很好用的日志宏。OceanBase 代码里面到处都有的 LOG_WARN 等宏就是在 ob_log.h 提供的。它的接口综合了 printf 和 cout 的优点，没有 cout 那么丑，又是强类型的，且限定了统一的 key-value 风格。为了在 C++ 老版实现这组接口，我们用了很多模板和宏的奇技淫巧。如果你先熟悉这组接口，再尝试贡献代码，你会爱上他们（这可是调试分布式系统的“小米加步枪”）。