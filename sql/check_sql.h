#ifndef CHECK_SQL_INCLUDED
#define CHECK_SQL_INCLUDED

#include "sql/sql_class.h"
#include "sql/parse_tree_nodes.h"
#include "sql/protocol.h"
#include <regex>
#include <vector>
#include <algorithm>
#include "sql/keyword_list.h" //这里的 keyword 由 sql/lex.h 生成


bool isFieldInList(const char* field, const char* list) {
    std::string pattern = "(^|,)" + std::string(field) + "(,|$)";
    std::regex r(pattern, std::regex::icase);
    return std::regex_search(list, r);
}
/*
// 分割字符串的函数
std::vector<char*> split(char* s, const char* delimiter) {
    std::vector<char*> tokens;
    char* token = strtok(s, delimiter);
    while (token != NULL) {
        tokens.push_back(token);
        token = strtok(NULL, delimiter);
    }
    return tokens;
}
// 检查字段是否在列表中
bool isFieldInList(char* field, char* list) {
    std::vector<char*> fields = split(list, ",");
    return std::find(fields.begin(), fields.end(), field) != fields.end();
}
*/
bool check_sql(THD *thd, PT_table_element *element, const char* db_name, const char* table_name) {
    // 如果 element 是索引
    if (dynamic_cast<PT_inline_index_definition*>(element)) {        
        PT_inline_index_definition *index = dynamic_cast<PT_inline_index_definition*>(element);
        // 检查索引中的字段个数
        if (sql_check_max_columns_per_index)
        {
            if (index->get_columns_count() > sql_check_max_columns_per_index) {
                const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                        .database_name = db_name,
                        .table_name = table_name,
                        .column = "",
                        .error = "索引 " + std::string(index->get_index_name().str) + " 字段个数为 " + std::to_string(index->get_columns_count()) + " ，超过 " + std::to_string(sql_check_max_columns_per_index),
                        .sql_info = strndup(index->m_pos.raw.start, index->m_pos.raw.length())
                    };
                    thd->lex->m_tmp_check_sql_results->push_back(error_info);
            }
        }
        // 检查是否有主键
        if (sql_check_need_primary)
        {
            if (index->get_keytype() == KEYTYPE_PRIMARY) {
                thd->lex->m_check_sql_has_primary = true;
                // 继续判断该主键字段是否整型
                List<PT_key_part_specification>* columns = index->get_m_columns();
                for (PT_key_part_specification &kp : *columns) {
                    //std::cout<<kp.get_column_name().str<<std::endl;
                    if ( thd->lex->m_sql_check_columns_type.find(kp.get_column_name().str) != thd->lex->m_sql_check_columns_type.end() ) {
                        if (!(thd->lex->m_sql_check_columns_type[kp.get_column_name().str] == MYSQL_TYPE_TINY ||
                            thd->lex->m_sql_check_columns_type[kp.get_column_name().str] == MYSQL_TYPE_SHORT ||
                            thd->lex->m_sql_check_columns_type[kp.get_column_name().str] == MYSQL_TYPE_LONG ||
                            thd->lex->m_sql_check_columns_type[kp.get_column_name().str] == MYSQL_TYPE_LONGLONG ||
                            thd->lex->m_sql_check_columns_type[kp.get_column_name().str] == MYSQL_TYPE_INT24)) {
                            const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                                .database_name = db_name,
                                .table_name = table_name,
                                .column = kp.get_column_name().str,
                                .error = "主键字段要求是整型(可短可长)",
                                .sql_info = strndup(index->m_pos.raw.start, index->m_pos.raw.length())
                            };
                            thd->lex->m_tmp_check_sql_results->push_back(error_info);
                        }
                    }
                }
            }
        }
        // 统计索引的个数
        thd->lex->m_check_sql_index_count++;
    }
    // 如果 element 是字段
    if (dynamic_cast<PT_column_def*>(element)) {
        // 把 函数参数中的 PT_table_element 类型向下转换为 PT_column_def 类型
        PT_column_def *column = dynamic_cast<PT_column_def*>(element);
        //记录字段类型到 m_sql_check_columns_type 中
        thd->lex->m_sql_check_columns_type[column->get_field_ident().str] = column->get_field_def()->type;
        // 检查是否有主键
        if (sql_check_need_primary)
        {
            if ((column->get_field_def()->type_flags & PRI_KEY_FLAG)){
                thd->lex->m_check_sql_has_primary = true;
                // 继续判断该主键字段是否整型
                if (sql_check_need_primary_number)
                    if (!(column->get_field_def()->type == MYSQL_TYPE_TINY || column->get_field_def()->type == MYSQL_TYPE_SHORT || 
                        column->get_field_def()->type == MYSQL_TYPE_LONG || column->get_field_def()->type == MYSQL_TYPE_LONGLONG || 
                        column->get_field_def()->type == MYSQL_TYPE_INT24)) {
                            const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                                .database_name = db_name,
                                .table_name = table_name,
                                .column = column->get_field_ident().str,
                                .error = "主键字段要求是整型(可短可长)",
                                .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
                            };
                            thd->lex->m_tmp_check_sql_results->push_back(error_info);
                    }
            }
        }
        // 检查字段是否允许为 NULL
        if (!sql_check_allow_null)
            if ((column->get_field_def()->type_flags & NOT_NULL_FLAG) != 1) {
                const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = column->get_field_ident().str,
                    .error = "该字段允许为 NULL",
                    .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
                };
                thd->lex->m_tmp_check_sql_results->push_back(error_info);
                //-exec p *(((Mem_root_array<const m_tmp_check_sql_result *> *)thd->lex->m_tmp_check_sql_results)->m_array[0])
                //-exec p *(((Mem_root_array<const m_tmp_check_sql_result *> *)thd->lex->m_tmp_check_sql_results)->m_array[1])
                //-exec p *(PT_key_part_specification*)((*(((PT_inline_index_definition*)(element))->m_columns)->first)->next.info)
            }
        // 检查字段是否允许为 unsigned
        if (!sql_check_allow_unsigned)
            if ((column->get_field_def()->type_flags & UNSIGNED_FLAG)) {
                const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = column->get_field_ident().str,
                    .error = "该字段允许为 unsigned",
                    .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
                };
                thd->lex->m_tmp_check_sql_results->push_back(error_info);
            }
        // 检查 varchar 字段是否超长
        if ((column->get_field_def()->type) == MYSQL_TYPE_STRING && atoi(column->get_field_def()->length) > sql_check_char_max_length)
        {
            const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = db_name,
                .table_name = table_name,
                .column = column->get_field_ident().str,
                .error = "该char类型字段长度为 " + std::to_string(atoi(column->get_field_def()->length)) + " ,超过 " + std::to_string(sql_check_char_max_length),
                .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
            };
            thd->lex->m_tmp_check_sql_results->push_back(error_info);
        }
        // 检查字段是否有注释
        if (sql_check_column_need_comment)
            if ((column->get_field_def()->comment.length) == 0) {
                const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = column->get_field_ident().str,
                    .error = "该字段没有 comment",
                    .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
                };
                thd->lex->m_tmp_check_sql_results->push_back(error_info);
            }
        // 检查时间类型字段是否有默认值
        if (sql_check_column_need_default_value && (column->get_field_def()->type == MYSQL_TYPE_TIMESTAMP || column->get_field_def()->type == MYSQL_TYPE_DATE
                                                 || column->get_field_def()->type == MYSQL_TYPE_TIME || column->get_field_def()->type == MYSQL_TYPE_DATETIME
                                                 || column->get_field_def()->type == MYSQL_TYPE_YEAR || column->get_field_def()->type == MYSQL_TYPE_NEWDATE
                                                 || column->get_field_def()->type == MYSQL_TYPE_TIMESTAMP2 || column->get_field_def()->type == MYSQL_TYPE_DATETIME2
                                                 || column->get_field_def()->type == MYSQL_TYPE_TIME || column->get_field_def()->type == MYSQL_TYPE_TIME)
                                                 && (column->get_field_def()->default_value == nullptr || column->get_field_def()->default_value->is_str_value_empty()))
        {
            //(*column->get_field_def())->default_value->str_value->m_ptr
            //(*column->get_field_def())->default_value->str_value->m_length 
            const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = db_name,
                .table_name = table_name,
                .column = column->get_field_ident().str,
                .error = "该时间类型字段没有默认值",
                .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
            };
            thd->lex->m_tmp_check_sql_results->push_back(error_info);
        }
        // 处理建表必须拥有的字段
        if (thd->lex->m_need_columns_map.find(column->get_field_ident().str) != thd->lex->m_need_columns_map.end() ){
            thd->lex->m_need_columns_map[column->get_field_ident().str] = true;
        }
        // 处理关键字
        if (!sql_check_allow_keyword) {
            for(auto x:keyword_list){
                if (x.reserved && strcasecmp(x.word,column->get_field_ident().str) == 0){
                    // 如果该关键字段不在 sql_check_allowed_keywords 中
                    if(!(strlen(sql_check_allowed_keywords) > 0 && isFieldInList(x.word, sql_check_allowed_keywords))){
                        const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                            .database_name = db_name,
                            .table_name = table_name,
                            .column = column->get_field_ident().str,
                            .error = "该字段是数据库系统关键字 " + std::string(x.word),
                            .sql_info = strndup(column->m_pos.raw.start, column->m_pos.raw.length())
                        };
                        thd->lex->m_tmp_check_sql_results->push_back(error_info);
                    }
                }
            }
        }
    }
    return true;
}

bool check_sql_extern(THD *thd, const char* db_name, const char* table_name) {
    if (sql_check_need_primary && thd->lex->m_check_sql_has_primary == false) {
        const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = "",
                    .error = "没有主键",
                    .sql_info = ""
                };
        thd->lex->m_tmp_check_sql_results->push_back(error_info);
    }
    if (thd->lex->m_check_sql_index_count > sql_check_max_indexs_per_table) {
        const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = "",
                    .error = "当前索引个数为 " + std::to_string(thd->lex->m_check_sql_index_count) + " ,超过 " + std::to_string(sql_check_max_indexs_per_table),
                    .sql_info = ""
                };
        thd->lex->m_tmp_check_sql_results->push_back(error_info);
    }
    //处理表名最大长度
    if (strlen(table_name) > static_cast<size_t>(sql_check_table_name_length)) {
        const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                    .database_name = db_name,
                    .table_name = table_name,
                    .column = "",
                    .error = "当前表名长度为 " + std::to_string(strlen(table_name)) + " ,超过 " + std::to_string(sql_check_table_name_length),
                    .sql_info = ""
                };
        thd->lex->m_tmp_check_sql_results->push_back(error_info);
    }
    //处理必有字段
    if (strlen(sql_check_need_columns_list)>0)
    {
        for (const auto &pair : thd->lex->m_need_columns_map){
            if (pair.second == false) {
                const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                            .database_name = db_name,
                            .table_name = table_name,
                            .column = "",
                            .error = std::string("当前表缺少必须拥有字段 ") + pair.first,
                            .sql_info = ""
                        };
                thd->lex->m_tmp_check_sql_results->push_back(error_info);
            }
        }
    }
    return true;
}

bool check_sql_dml_has_no_where(THD *thd) {
    const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = "",
                .table_name = "",
                .column = "",
                .error = "DML need where",
                .sql_info = thd->query().str
            };
    thd->lex->m_tmp_check_sql_results->push_back(error_info);
    return true;
}

bool check_sql_dml_not_allowed_select(THD *thd) {
    const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = "",
                .table_name = "",
                .column = "",
                .error = "DML not allowed select",
                .sql_info = thd->query().str
            };
    thd->lex->m_tmp_check_sql_results->push_back(error_info);
    return true;
}

bool check_sql_dml_not_allowed_order(THD *thd) {
    const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = "",
                .table_name = "",
                .column = "",
                .error = "DML not allowed order",
                .sql_info = thd->query().str
            };
    thd->lex->m_tmp_check_sql_results->push_back(error_info);
    return true;
}

bool check_sql_dml_need_column(THD *thd) {
    const m_tmp_check_sql_result *error_info = new m_tmp_check_sql_result{
                .database_name = "",
                .table_name = "",
                .column = "",
                .error = "INSERT need column",
                .sql_info = thd->query().str
            };
    thd->lex->m_tmp_check_sql_results->push_back(error_info);
    return true;
}
#endif /* CHECK_SQL_INCLUDED */