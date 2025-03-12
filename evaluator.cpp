
#include "evaluator.h"
#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"

extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static void eval_create_table(const create_table* info);
static void eval_select_from(const select_from* info);
static void print_table(table tab);
static void eval_insert_into(const insert_into* info);

#define eval_push_error_return(x)               \
        std::string error = x;                  \
        errors.push_back(error);                \
        return                                  \

void eval_init(std::vector<node*> nds) {
    nodes = nds;
}

void eval() {
    
    for (int i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type() == std::string("insert_into")) {
            eval_insert_into(static_cast<insert_into*>(nodes[i]));
            printf("EVAL INSERT INTO CALLED\n");
        } 
        else if  (nodes[i]->type() == std::string("select_from")) {
            eval_select_from(static_cast<select_from*>(nodes[i]));
            printf("EVAL SELECT FROM CALLED\n");
        } 
        else if (nodes[i]->type() == std::string("create_table")) {
            eval_create_table(static_cast<create_table*>(nodes[i]));
            printf("EVAL CREATE TABLE CALLED\n");
        }
        else {
            errors.push_back("eval: unknown node type (" + nodes[i]->type() + ")");
            return;
        }
    }
}

static void eval_create_table(const create_table* info) {

    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            errors.push_back("Table already exists");
            return;
        }
    }

    table tab;
    tab.name = info->table_name;
    tab.column_datas = info->column_datas;

    tables.push_back(tab);
}

static void eval_select_from(const select_from* info) {

    table tab;
    bool found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab = tables[i];
            found = true;
            break;}
    }
    
    if (!found) {
        errors.push_back("eval_select_from(): Table not found");
        return;}

    std::cout << info->table_name << ":\n";

    print_table(tab);

    // For Qt
    display_tab.to_display = true;
    display_tab.tab = tab;
}

static void print_table(table tab) {

    for (int i = 0; i < tab.column_datas.size(); i++) {
        std::string name = tab.column_datas[i].field_name;
        std::string to_print = name.substr(0, 8);
        int pad_length = 8 - to_print.length();
        std::string pad(pad_length, ' ');
        std::string out_string = to_print + pad + " | ";
        std::cout << out_string;
    }

    std::cout << std::endl;

    for (int i = 0; i < tab.rows.size(); i++) {
        for(int j = 0; j < tab.rows[0].column_values.size(); j++) {
            std::string name = tab.rows[i].column_values[j];
            std::string to_print = name.substr(0, 8);
            int pad_length = 8 - to_print.length();
            std::string pad(pad_length, ' ');
            std::string out_string = to_print + pad + " | ";
            std::cout << out_string;
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

static void eval_insert_into(const insert_into* info) {

    table* tab_ptr;
    bool table_found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab_ptr = &tables[i];
            table_found = true;
            break;}
    }

    if (info->values.size() == 0) {
        eval_push_error_return("INSERT INTO, no values");}

    if (!table_found) {
        eval_push_error_return("INSERT INTO, table not found");}

    if (info->field_names.size() < info->values.size()) {
        eval_push_error_return("INSET INTO, more values than field names");}

    if (info->field_names.size() > tab_ptr->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more field names than table has columns");}

    row roh;
    int j = 0;
    if (info->field_names.size() > 0) {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {
            if (tab_ptr->column_datas[i].field_name == info->field_names[j]) {
                data_type_pair pair = info->values[j];
                if (pair.type == INTEGER_LITERAL) {
                    if (!is_integer_data_type(tab_ptr->column_datas[i].data_type)) {
                        errors.push_back("eval_insert_into(): Value: (" + pair.data + ") has mismatching type with column (" + tab_ptr->column_datas[i].data_type + ")");
                        return;
                    }
                } else if (pair.type == STRING_LITERAL) {
                    if (!is_string_data_type(tab_ptr->column_datas[i].data_type)) {
                        errors.push_back("eval_insert_into(): Value: (" + pair.data + ") has mismatching type with column(" + tab_ptr->column_datas[i].data_type + ")");
                        return;
                    }
                } else {
                    errors.push_back("eval_insert_into(): Value: (" + pair.data + "), type: (" + keyword_enum_to_string(pair.type) + ") probably shouldn't be able to insert this");
                    return;
                }
                roh.column_values.push_back(info->values[j].data);
                j++;
                continue;
            }
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value);
        }
    } else {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {
            if (j < info->values.size()) {
                roh.column_values.push_back(info->values[j].data);
                j++;
                continue;
            }
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value);
        }
    }

    if (j < info->values.size()) {
        std::string err = "INSERT INTO, field name (" + info->field_names[j] + ") not found";
        eval_push_error_return(err);
    }

    if (j < info->values.size()) {
        eval_push_error_return("eval_insert_into(): werid bug");
    }

    tab_ptr->rows.push_back(roh);
}