ALTER TABLE table_name ADD INDEX index_name (column_list)
ALTER TABLE table_name DROP INDEX index_name



ALTER TABLE collection_info_FF ADD INDEX userid_taskid (user_id,task_id);
ALTER TABLE collection_info_FF DROP INDEX userid_taskid;

select *from collection_info_FF limit 1;
+---------------+-----------+----------------------------------+--------------+----------+-----------+---------------+-------------+----------------+---------------------+-------------+
| collection_id | user_id   | md5                              | filename     | filetype | folder_id | task_id       | client_name | client_version | create_time         | modify_time |
+---------------+-----------+----------------------------------+--------------+----------+-----------+---------------+-------------+----------------+---------------------+-------------+
| 1000000109901 | 666825034 | FEDEF66110298F7F0CA42ACDD7A191E7 | pv7y2nt.rmvb |        4 |     10000 | 1003067136915 | xl_pc_web   |              0 | 2017-07-08 15:47:30 | NULL        |
+---------------+-----------+----------------------------------+--------------+----------+-----------+---------------+-------------+----------------+---------------------+-------------

show index from collection_info_FF;        
+--------------------+------------+---------------+--------------+---------------+-----------+-------------+----------+--------+------+------------+---------+---------------+
| Table              | Non_unique | Key_name      | Seq_in_index | Column_name   | Collation | Cardinality | Sub_part | Packed | Null | Index_type | Comment | Index_comment |
+--------------------+------------+---------------+--------------+---------------+-----------+-------------+----------+--------+------+------------+---------+---------------+
| collection_info_FF |          0 | PRIMARY       |            1 | collection_id | A         |        2436 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_filetype |            1 | user_id       | A         |         598 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_filetype |            2 | filetype      | A         |         654 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_folder   |            1 | user_id       | A         |         598 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_folder   |            2 | folder_id     | A         |         598 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_md5      |            1 | user_id       | A         |         598 |     NULL | NULL   |      | BTREE      |         |               |
| collection_info_FF |          1 | user_md5      |            2 | md5           | A         |        2385 |     NULL | NULL   |      | BTREE      |         |               |
+--------------------+------------+---------------+--------------+---------------+-----------+-------------+----------+--------+------+------------+---------+---------------+





explain select collection_id, md5, filename, filetype, folder_id, task_id, create_time from collection_info_FF  where user_id = 666825034 and  folder_id = 10000  and task_id in (1003067136915, 0000);









explain select collection_id, md5, filename, filetype, folder_id, task_id, create_time from collection_info_FF  where user_id = 666825034 and  folder_id = 10000  and task_id in (1003067136915, 0000); 
+----+-------------+--------------------+------------+-------+--------------------------------------------------+---------------+---------+------+------+----------+------------------------------------+
| id | select_type | table              | partitions | type  | possible_keys                                    | key           | key_len | ref  | rows | filtered | Extra                              |
+----+-------------+--------------------+------------+-------+--------------------------------------------------+---------------+---------+------+------+----------+------------------------------------+
|  1 | SIMPLE      | collection_info_FF | NULL       | range | user_filetype,user_folder,user_md5,userid_taskid | userid_taskid | 16      | NULL |    2 |    10.00 | Using index condition; Using where |
+----+-------------+--------------------+------------+-------+--------------------------------------------------+---------------+---------+------+------+----------+------------------------------------+

explain select collection_id, md5, filename, filetype, folder_id, task_id, create_time from collection_info_FF  where user_id = 666825034 and  folder_id = 1000  and task_id in (1003067136915, 0000); 
+----+-------------+--------------------+------------+------+--------------------------------------------------+-------------+---------+-------------+------+----------+-------------+
| id | select_type | table              | partitions | type | possible_keys                                    | key         | key_len | ref         | rows | filtered | Extra       |
+----+-------------+--------------------+------------+------+--------------------------------------------------+-------------+---------+-------------+------+----------+-------------+
|  1 | SIMPLE      | collection_info_FF | NULL       | ref  | user_filetype,user_folder,user_md5,userid_taskid | user_folder | 16      | const,const |    1 |    20.00 | Using where |
+----+-------------+--------------------+------------+------+--------------------------------------------------+-------------+---------+-------------+------+----------+-------------+




explain select collection_id, md5, filename, filetype, folder_id, task_id, create_time from collection_info_FF  where user_id = 666825034 and  folder_id = 10000  and task_id in (1003067136915, 0000);
+----+-------------+--------------------+------------+------+------------------------------------+---------------+---------+-------+------+----------+-------------+
| id | select_type | table              | partitions | type | possible_keys                      | key           | key_len | ref   | rows | filtered | Extra       |
+----+-------------+--------------------+------------+------+------------------------------------+---------------+---------+-------+------+----------+-------------+
|  1 | SIMPLE      | collection_info_FF | NULL       | ref  | user_filetype,user_folder,user_md5 | user_filetype | 8       | const |   14 |     2.00 | Using where |
+----+-------------+--------------------+------------+------+------------------------------------+---------------+---------+-------+------+----------+-------------+

explain select collection_id, md5, filename, filetype, folder_id, task_id, create_time from collection_info_FF  where user_id = 666825034 and  folder_id = 1000  and task_id in (1003067136915, 0000);
+----+-------------+--------------------+------------+------+------------------------------------+-------------+---------+-------------+------+----------+-------------+
| id | select_type | table              | partitions | type | possible_keys                      | key         | key_len | ref         | rows | filtered | Extra       |
+----+-------------+--------------------+------------+------+------------------------------------+-------------+---------+-------------+------+----------+-------------+
|  1 | SIMPLE      | collection_info_FF | NULL       | ref  | user_filetype,user_folder,user_md5 | user_folder | 16      | const,const |    1 |    20.00 | Using where |
+----+-------------+--------------------+------------+------+------------------------------------+-------------+---------+-------------+------+----------+-------------+







ALTER TABLE task_info_00 ADD INDEX create_time (create_time);
ALTER TABLE task_info_00 DROP INDEX create_time;

ALTER TABLE task_info_00 ADD INDEX userid_create_time (user_id,create_time);
ALTER TABLE task_info_00 DROP INDEX userid_create_time;


show index from task_info_00;
+--------------+------------+------------------+--------------+-------------+-----------+-------------+----------+--------+------+------------+---------+---------------+
| Table        | Non_unique | Key_name         | Seq_in_index | Column_name | Collation | Cardinality | Sub_part | Packed | Null | Index_type | Comment | Index_comment |
+--------------+------------+------------------+--------------+-------------+-----------+-------------+----------+--------+------+------------+---------+---------------+
| task_info_00 |          0 | PRIMARY          |            1 | task_id     | A         |       76035 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_filetype    |            1 | user_id     | A         |         357 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_filetype    |            2 | filetype    | A         |        1438 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_flag_create |            1 | user_id     | A         |         432 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_flag_create |            2 | flag        | A         |         671 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_flag_create |            3 | create_time | A         |       70565 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_md5         |            1 | user_id     | A         |         517 |     NULL | NULL   |      | BTREE      |         |               |
| task_info_00 |          1 | user_md5         |            2 | md5         | A         |       73852 |     NULL | NULL   |      | BTREE      |         |               |
+--------------+------------+------------------+--------------+-------------+-----------+-------------+----------+--------+------+------------+---------+---------------+

explain select distinct(user_id) from task_info_00 where create_time < '20190101';
+----+-------------+--------------+------------+-------+-----------------------------------------+------------------+---------+------+-------+----------+--------------------------+
| id | select_type | table        | partitions | type  | possible_keys                           | key              | key_len | ref  | rows  | filtered | Extra                    |
+----+-------------+--------------+------------+-------+-----------------------------------------+------------------+---------+------+-------+----------+--------------------------+
|  1 | SIMPLE      | task_info_00 | NULL       | index | user_filetype,user_flag_create,user_md5 | user_flag_create | 15      | NULL | 76036 |    33.33 | Using where; Using index |
+----+-------------+--------------+------------+-------+-----------------------------------------+------------------+---------+------+-------+----------+--------------------------+



explain select distinct(user_id) from task_info_00 where create_time < '20190101';
+----+-------------+--------------+------------+-------+------------------------------------------------------------+--------------------+---------+------+-------+----------+--------------------------+
| id | select_type | table        | partitions | type  | possible_keys                                              | key                | key_len | ref  | rows  | filtered | Extra                    |
+----+-------------+--------------+------------+-------+------------------------------------------------------------+--------------------+---------+------+-------+----------+--------------------------+
|  1 | SIMPLE      | task_info_00 | NULL       | index | user_filetype,user_flag_create,user_md5,userid_create_time | userid_create_time | 13      | NULL | 76035 |    33.33 | Using where; Using index |
+----+-------------+--------------+------------+-------+------------------------------------------------------------+--------------------+---------+------+-------+----------+--------------------------+



explain select * from task_info_00 where user_id = 608520547 and create_time < '20190101';
+----+-------------+--------------+------------+------+-----------------------------------------+---------------+---------+-------+------+----------+-------------+
| id | select_type | table        | partitions | type | possible_keys                           | key           | key_len | ref   | rows | filtered | Extra       |
+----+-------------+--------------+------------+------+-----------------------------------------+---------------+---------+-------+------+----------+-------------+
|  1 | SIMPLE      | task_info_00 | NULL       | ref  | user_filetype,user_flag_create,user_md5 | user_filetype | 8       | const |   93 |    33.33 | Using where |
+----+-------------+--------------+------------+------+-----------------------------------------+---------------+---------+-------+------+----------+-------------+

explain select * from task_info_00 where user_id = 608520547 and create_time < '20190101';
+----+-------------+--------------+------------+------+------------------------------------------------------------+---------------+---------+-------+------+----------+-------------+
| id | select_type | table        | partitions | type | possible_keys                                              | key           | key_len | ref   | rows | filtered | Extra       |
+----+-------------+--------------+------------+------+------------------------------------------------------------+---------------+---------+-------+------+----------+-------------+
|  1 | SIMPLE      | task_info_00 | NULL       | ref  | user_filetype,user_flag_create,user_md5,userid_create_time | user_filetype | 8       | const |   93 |    33.33 | Using where |
+----+-------------+--------------+------------+------+------------------------------------------------------------+---------------+---------+-------+------+----------+-------------+


