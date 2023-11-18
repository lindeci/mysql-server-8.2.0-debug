#ifndef CHECK_SQL_SEND_INCLUDED
#define CHECK_SQL_SEND_INCLUDED

#include "sql/protocol.h"
#include "sql/sql_class.h"

// SQL 审核时，返回给客户端的元数据
// 如果不添加 inline，则会报错：下面两个函数在 sql/check_sql_send.h 文件中被定义，同时被 sql_cmd_ddl_table.cc 和 sql_union.cc 引用了，这导致了多次定义。
inline bool check_sql_send_field_metadata(THD *thd) {
    thd->get_protocol()->start_result_metadata(5, Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF, thd->variables.character_set_results);    

    Send_field* my_send_field = new Send_field();
    my_send_field->db_name = "test";
    my_send_field->table_name = "check_sql";
    my_send_field->org_table_name = "check_sql";
    my_send_field->length = 256;
    my_send_field->charsetnr = 255;
    my_send_field->flags = 4097;
    my_send_field->decimals = 0;
    my_send_field->type = MYSQL_TYPE_VARCHAR;
    my_send_field->field = true;

    
    my_send_field->col_name = "schema_name";
    my_send_field->org_col_name = "table_name";
    thd->get_protocol()->start_row();
    thd->get_protocol()->send_field_metadata(my_send_field, thd->variables.character_set_results);
    thd->get_protocol()->end_row();

    my_send_field->col_name = "table_name";
    my_send_field->org_col_name = "table_name";
    thd->get_protocol()->start_row();
    thd->get_protocol()->send_field_metadata(my_send_field, thd->variables.character_set_results);
    thd->get_protocol()->end_row();

    my_send_field->col_name = "column_name";
    my_send_field->org_col_name = "column_name";
    thd->get_protocol()->start_row();
    thd->get_protocol()->send_field_metadata(my_send_field, thd->variables.character_set_results);
    thd->get_protocol()->end_row();

    my_send_field->col_name = "error_info";
    my_send_field->org_col_name = "error_info";
    thd->get_protocol()->start_row();
    thd->get_protocol()->send_field_metadata(my_send_field, thd->variables.character_set_results);
    thd->get_protocol()->end_row();

    my_send_field->col_name = "sql_info";
    my_send_field->org_col_name = "sql_info";
    thd->get_protocol()->start_row();
    thd->get_protocol()->send_field_metadata(my_send_field, thd->variables.character_set_results);
    thd->get_protocol()->end_row();

    thd->get_protocol()->end_result_metadata();
    return true;
}

inline bool check_sql_send_data(THD *thd) {

    for (auto item = thd->lex->m_tmp_check_sql_results->begin(); item != thd->lex->m_tmp_check_sql_results->end(); item++) {
        thd->get_protocol()->start_row();
        thd->get_protocol()->store_string((*item)->database_name, strlen((*item)->database_name), thd->variables.character_set_results);
        thd->get_protocol()->store_string((*item)->table_name, strlen((*item)->table_name), thd->variables.character_set_results);
        thd->get_protocol()->store_string((*item)->column, strlen((*item)->column), thd->variables.character_set_results);
        thd->get_protocol()->store_string((*item)->error.c_str(), strlen((*item)->error.c_str()), thd->variables.character_set_results);
        thd->get_protocol()->store_string((*item)->sql_info, strlen((*item)->sql_info), thd->variables.character_set_results);
        thd->inc_sent_row_count(1);
        thd->get_protocol()->end_row();
    }
    return true;
}
#endif /* CHECK_SQL_SEND_INCLUDED */