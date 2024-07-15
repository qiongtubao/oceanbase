
# parser 
arser 模块执行语法分析，把 SQL 字符串解析为一个 ParseNode 组成的抽象语法树。他的接口类是 ObParser 类。
parser 是由 bison 和 flex 生成的 C 语言代码，OceanBase 的 C 语言代码位于这里。  
1. parse_node.c  
2. sql_parser_base.c, 
3. sql_parser_mysql_mode_lex.c
4. sql_parser_mysql_mode_tabl.c
5. type_name.c
parser 有一种快速解析模式，目标不是产生语法树，而且把 SQL 字符串参数化，具体原理我们将在之后的系列博客中解释。