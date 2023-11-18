- [概述](#概述)
- [1. 控制 SQL 审核的全局系统变量](#1-控制-sql-审核的全局系统变量)
  - [1.1 默认的 SQL 审核系统变量](#11-默认的-sql-审核系统变量)
  - [1.2 变量介绍](#12-变量介绍)
- [2. 测试用例](#2-测试用例)
  - [2.1 开启极端的 SQL 审核规则](#21-开启极端的-sql-审核规则)
  - [2.2 建表字段测试用例](#22-建表字段测试用例)
  - [2.3 建表索引测试](#22-建表索引测试)
  - [2.4 DML 测试](#23-dml-测试)
- [3. 所有的开发细节](#3-所有的开发细节)

# 概述
基于 MySQL 8.2.0 源码开发的 SQL 审核工具，100.000% 兼容 MySQL 语法。  
在对 MySQL 语法兼容性方面，与市面上任何同款工具对比，`遥遥领先！`

[该工具的 docker 安装方法](https://github.com/lindeci/mysql-server-8.2.0-docker) ： `https://github.com/lindeci/mysql-server-8.2.0-docker`

# 1. 控制 SQL 审核的全局系统变量
## 1.1 默认的 SQL 审核系统变量
```sql
mysql> show variables like 'sql_check%';
+-------------------------------------+-------+
| Variable_name                       | Value |
+-------------------------------------+-------+
| sql_check                           | OFF   |
| sql_check_allow_drop_database       | ON    |
| sql_check_allow_drop_table          | ON    |
| sql_check_allow_keyword             | OFF   |
| sql_check_allow_null                | OFF   |
| sql_check_allow_unsigned            | OFF   |
| sql_check_allowed_keywords          |       |
| sql_check_char_max_length           | 255   |
| sql_check_column_need_comment       | ON    |
| sql_check_column_need_default_value | ON    |
| sql_check_dml_allow_order           | ON    |
| sql_check_dml_allow_select          | OFF   |
| sql_check_dml_need_where            | ON    |
| sql_check_insert_need_column        | ON    |
| sql_check_max_columns_per_index     | 6     |
| sql_check_max_indexs_per_table      | 8     |
| sql_check_need_columns_list         |       |
| sql_check_need_primary              | ON    |
| sql_check_need_primary_number       | ON    |
| sql_check_table_name_length         | 64    |
+-------------------------------------+-------+
20 rows in set (0.02 sec)
```
## 1.2 变量介绍
| 全局变量名称                         | 变量介绍                                          |
| ----------------------------------- | ------------------------------------------------ |
| sql_check                           | SQL 审核：总开关                                  |
| sql_check_allow_drop_database       | SQL 审核：是否允许 drop database                  |
| sql_check_allow_drop_table          | SQL 审核：是否允许 drop table                     |
| sql_check_allow_keyword             | SQL 审核：建表字段是否允许使用关键字                |
| sql_check_allow_null                | SQL 审核：建表的字符字段是否允许 null               |
| sql_check_allow_unsigned            | SQL 审核：建表的整型字段是否允许 unsigned           |
| sql_check_allowed_keywords          | SQL 审核：建表的字段允许使用的关键字（逗号隔开）      |
| sql_check_char_max_length           | SQL 审核：建表的 char 字段最大长度                  |
| sql_check_column_need_comment       | SQL 审核：建表的字段是否需要注释                    |
| sql_check_column_need_default_value | SQL 审核：建表的时间字段是否需要默认值               |
| sql_check_dml_allow_order           | SQL 审核：是否允许 update/delete ... order by ...  |
| sql_check_dml_allow_select          | SQL 审核：是否允许 insert ... select ...           |
| sql_check_dml_need_where            | SQL 审核：update/delete 是否要求有 where 条件       |
| sql_check_insert_need_column        | SQL 审核：insert 语句是否需要指定字段               |
| sql_check_max_columns_per_index     | SQL 审核：建表时每个索引允许最多允许多少个字段        |
| sql_check_max_indexs_per_table      | SQL 审核：建表时每张表最多有几个索引（含主键）        |
| sql_check_need_columns_list         | SQL 审核：建表时必须拥有的字段（逗号隔开）           |
| sql_check_need_primary              | SQL 审核：检查建表是否有主键                        |
| sql_check_need_primary_number       | SQL 审核：建表时主键是否要求是整型                   |
| sql_check_table_name_length         | SQL 审核：建表时表名的最大长度                       |

# 2. 测试用例
下面的测试用例覆盖所有的 SQL 审核系统变量
## 2.1 开启极端的 SQL 审核规则
```sql
set global sql_check = 1;
set global sql_check_max_indexs_per_table = 1;
set global sql_check_max_columns_per_index = 1;
set global sql_check_char_max_length = 1;
set global sql_check_table_name_length = 1;
set global sql_check_allowed_keywords='as,select,begin';
set global sql_check_need_columns_list='delete_flag,update_time';
set global sql_check_allow_drop_table = 0;
set global sql_check_allow_drop_database = 0;
set global sql_check_dml_allow_select = 0;
set global sql_check_dml_allow_order = 0;
set global sql_check_dml_need_where = 1;
set global sql_check_insert_need_column = 1;
set global sql_check_allow_unsigned = 0;
```
注：如果关闭SQL 审核 `set global sql_check = 1` ，则该工具是原生版本的 MySQL 服务端。
## 2.2 建表字段测试用例
```sql
mysql> create table test(id int unsigned, c1 char(5), dt timestamp, `select` int,`INSERT` blob); 
+-------------+------------+-------------+-----------------------------------------------+-----------------+
| schema_name | table_name | column_name | error_info                                    | sql_info        |
+-------------+------------+-------------+-----------------------------------------------+-----------------+
| test        | test       | id          | 该字段允许为 NULL                             | id int unsigned |
| test        | test       | id          | 该字段允许为 unsigned                         | id int unsigned |
| test        | test       | id          | 该字段没有 comment                            | id int unsigned |
| test        | test       | c1          | 该字段允许为 NULL                             | c1 char(5)      |
| test        | test       | c1          | 该char类型字段长度为 5 ,超过 1                | c1 char(5)      |
| test        | test       | c1          | 该字段没有 comment                            | c1 char(5)      |
| test        | test       | dt          | 该字段允许为 NULL                             | dt timestamp    |
| test        | test       | dt          | 该字段没有 comment                            | dt timestamp    |
| test        | test       | dt          | 该时间类型字段没有默认值                      | dt timestamp    |
| test        | test       | select      | 该字段允许为 NULL                             | `select` int    |
| test        | test       | select      | 该字段没有 comment                            | `select` int    |
| test        | test       | INSERT      | 该字段允许为 NULL                             | `INSERT` blob   |
| test        | test       | INSERT      | 该字段没有 comment                            | `INSERT` blob   |
| test        | test       | INSERT      | 该字段是数据库系统关键字 INSERT               | `INSERT` blob   |
| test        | test       |             | 没有主键                                      |                 |
| test        | test       |             | 当前表名长度为 4 ,超过 1                      |                 |
| test        | test       |             | 当前表缺少必须拥有字段 update_time            |                 |
| test        | test       |             | 当前表缺少必须拥有字段 delete_flag            |                 |
+-------------+------------+-------------+-----------------------------------------------+-----------------+
18 rows in set (0.00 sec)
```
## 2.3 建表索引测试
```sql
create table t (
    id varchar(5) primary key not null comment "id", 
    c1 int not null comment "c1", 
    update_time int not null comment "update_time",
    delete_flag int not null comment "delete_flag", 
    index idx1(id), 
    index idx2(c1)
);
+-------------+------------+-------------+-------------------------------------------+-------------------------------------------------+
| schema_name | table_name | column_name | error_info                                | sql_info                                        |
+-------------+------------+-------------+-------------------------------------------+-------------------------------------------------+
| test        | t          | id          | 主键字段要求是整型(可短可长)              | id varchar(5) primary key not null comment "id" |
| test        | t          |             | 当前索引个数为 2 ,超过 1                  |                                                 |
+-------------+------------+-------------+-------------------------------------------+-------------------------------------------------+
2 rows in set (0.00 sec)

```
## 2.4 DML 测试
```sql
mysql> delete from a order by c;           
+-------------+------------+-------------+-----------------------+--------------------------+
| schema_name | table_name | column_name | error_info            | sql_info                 |
+-------------+------------+-------------+-----------------------+--------------------------+
|             |            |             | DML need where        | delete from a order by c |
|             |            |             | DML not allowed order | delete from a order by c |
+-------------+------------+-------------+-----------------------+--------------------------+
2 rows in set (0.00 sec)

mysql> update a set id=9 order by c;
+-------------+------------+-------------+-----------------------+------------------------------+
| schema_name | table_name | column_name | error_info            | sql_info                     |
+-------------+------------+-------------+-----------------------+------------------------------+
|             |            |             | DML need where        | update a set id=9 order by c |
|             |            |             | DML not allowed order | update a set id=9 order by c |
+-------------+------------+-------------+-----------------------+------------------------------+
2 rows in set (0.00 sec)

mysql> insert into a select 1;
+-------------+------------+-------------+------------------------+------------------------+
| schema_name | table_name | column_name | error_info             | sql_info               |
+-------------+------------+-------------+------------------------+------------------------+
|             |            |             | DML not allowed select | insert into a select 1 |
|             |            |             | INSERT need column     | insert into a select 1 |
+-------------+------------+-------------+------------------------+------------------------+
2 rows in set (0.03 sec)
```

# 3. 所有的开发细节
基于 MySQL 8.2.0 源码开发。

`在这里你可以了解到该审核工具对 MySQL 8.2.0 源码 的所有改动。`

[该审核工具第一个工业级版本与 MySQL 8.2.0 源码的官方版本的代码对比](https://github.com/lindeci/mysql-server-8.2.0-debug/commit/d4847badb436863be38114c51b26fbaf2abb9eeb)：
`https://github.com/lindeci/mysql-server-8.2.0-debug/commit/d4847badb436863be38114c51b26fbaf2abb9eeb`
