
#include "evaluator.h"
#include "node.h"
#include "structs_and_macros.h"
#include "helpers.h"
#include "object.h"

extern std::vector<std::string> errors;
extern std::vector<table> tables;
extern display_table display_tab;

static std::vector<node*> nodes;

static void eval_alter_table(alter_table* info);
static void eval_create_table(const create_table* info);
static void eval_select_from(select_from* info);
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
        } else if  (nodes[i]->type() == std::string("alter_table")) {
            eval_alter_table(static_cast<alter_table*>(nodes[i]));
            printf("EVAL ALTER ATBLE CALLED\n");
        } else {
            errors.push_back("eval: unknown node type (" + nodes[i]->type() + ")");
            return;
        }
    }
}

static void eval_alter_table(alter_table* info) {
    table* tab;
    bool tab_found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab = &tables[i];
            tab_found = true;
            break;}
    }
    
    if (!tab_found) {
        errors.push_back("eval_select_from(): Table not found (" + info->table_name + ")");
        return;}

    switch (info->table_edit->type()) {
    case COLUMN_OBJ: {

        column_object* column_obj = static_cast<column_object*>(info->table_edit);

        for (int i = 0; i < tab->column_datas.size(); i++) {
            if (column_obj->column_name == tab->column_datas[i].field_name) {
                errors.push_back("eval_alter_table(): Table already contains column with name (" + column_obj->column_name + ")");
                return;
            }
        }

        column_data col = {column_obj->column_name, column_obj->data_type, column_obj->default_value};
        tab->column_datas.push_back(col);
    } break;
    default:
        errors.push_back("eval_alter_table(): Table edit not supported");
        return;
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


static bool eval_condtion(token op, object* left, object* right, std::vector<column_data> columns, row tab_row) {

    // Search for column
    int column_index = -1;
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i].field_name == left->inspect()) {
            column_index = i;
            break;
        }
    }

    std::string column_value = tab_row.column_values.at(column_index);

    SQL_data_type_object* type = columns.at(column_index).data_type;


    switch (op.type) {
    case EQUAL:
        if (column_value == right->data()) {
            return true;
            break;
        }

        std::cout << "CONDITION FAIL FOR (" + column_value + ", " + right->data() + ")" << std::endl;

        // errors.push_back("eval_select_from(): Unknown = operation for " + left->inspect() + ", " + right->inspect());
        return false;
        break;
    case GREATER_THAN:
        if (std::stoi(column_value) > std::stoi(right->data())) {
            return true;
            break;
        }
        return false;
        break;
    case LESS_THAN:
        if (std::stoi(column_value) < std::stoi(right->data())) {
            return true;
            break;
        }
        return false;
        break;
    default:
        errors.push_back("eval_select_from(): Unknown condition operator, (" + token_type_to_string(op.type) + ")");
        return false;
    }
}

static void eval_select_from(select_from* info) {

    table tab;
    bool found = false;
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == info->table_name) {
            tab = tables[i];
            found = true;
            break;}
    }
    
    if (!found) {
        errors.push_back("eval_select_from(): Table not found (" + info->table_name + ")");
        return;}

    if (info->asterisk == true) {
        for (int i = 0; i < tab.column_datas.size(); i++) {
            info->column_names.push_back(tab.column_datas[i].field_name); }
    }
    


    std::vector<int> row_ids;
    if (info->condition->type() != NULL_OBJ) {

        if (info->condition->type() != INFIX_EXPRESSION_OBJ) {
            errors.push_back("Unsupported condition type, (" + info->condition->inspect() + ")");
            return;
        }

        infix_expression_object* condition = static_cast<infix_expression_object*>(info->condition);


        token op = condition->op;
        object* left = condition->left;
        object* right = condition->right;

        for (int i = 0; i < tab.rows.size(); i++) {
            int error_count = errors.size();

            bool should_add = eval_condtion(op, left, right, tab.column_datas, tab.rows[i]);

            if (errors.size() > error_count) {
                return;}
            
            if (should_add) {
                row_ids.push_back(i);
            }
        }

    } else {
        for (int i = 0; i < tab.rows.size(); i++) {
            row_ids.push_back(i);
        }
    }

        


    display_tab.to_display = true;
    display_tab.tab = tab;
    display_tab.column_names = info->column_names;
    display_tab.row_ids = row_ids;

    print_table(tab); // CMD line print, QT will do it's own thing in main

}

static void print_table(table tab) {

    std::cout << tab.name << ":\n";
    // for (int i = 0; i < tab.column_datas.size(); i++) {
    //     std::string name = tab.column_datas[i].field_name;
    //     std::string to_print = name.substr(0, 8);
    //     int pad_length = 8 - to_print.length();
    //     std::string pad(pad_length, ' ');
    //     std::string out_string = to_print + pad + " | ";
    //     std::cout << out_string;
    // }

    std::string field_names = "";
    std::vector<int> row_indexes;
    int j = 0;
    for (int i = 0; i < display_tab.column_names.size(); i++) {
        while (tab.column_datas[j].field_name != display_tab.column_names[i]) {
            j++;}
        row_indexes.push_back(j);

        std::string full_name = tab.column_datas[j].field_name;

        std::string name = full_name.substr(0, 10);
        int pad_length = 10 - name.length();
        std::string pad(pad_length, ' ');
        name += pad + " | ";

        field_names += name;
    }

    std::cout << field_names <<std::endl;

    // for (int i = 0; i < tab.rows.size(); i++) {
    //     for(int j = 0; j < tab.rows[0].column_values.size(); j++) {
    //         std::string name = tab.rows[i].column_values[j];
    //         std::string to_print = name.substr(0, 8);
    //         int pad_length = 8 - to_print.length();
    //         std::string pad(pad_length, ' ');
    //         std::string out_string = to_print + pad + " | ";
    //         std::cout << out_string;
    //     }
    //     std::cout << std::endl;
    // }

    for (int i = 0; i < tab.rows.size(); i++) {
        std::string row_values = "";
        for (int j = 0; j < row_indexes.size(); j++) {

            std::string full_name = tab.rows[i].column_values[j];

            std::string name = full_name.substr(0, 10);
            int pad_length = 10 - name.length();
            std::string pad(pad_length, ' ');
            name += pad + " | ";

            row_values += name;
        }
        std::cout << row_values << std::endl;
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

    if (info->field_names.size() < info->values.size()) {
        eval_push_error_return("INSET INTO, more values than field names");}

    if (info->field_names.size() > tab_ptr->column_datas.size()) {
        eval_push_error_return("INSERT INTO, more field names than table has columns");}

    if (!table_found) { // More important errors should go last so basic mistakes are fixed first???
        eval_push_error_return("INSERT INTO, table not found");}

    row roh;
    int j = 0;
    if (info->field_names.size() > 0) {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {
            if (tab_ptr->column_datas[i].field_name == info->field_names[j]) {
                object* insert_obj = info->values[j];
                SQL_data_type_object* data_type = tab_ptr->column_datas[i].data_type;
                insert_obj = can_insert(insert_obj, data_type);
                if (insert_obj->type() == ERROR_OBJ) {
                    errors.push_back(insert_obj->data());
                    return;}
                roh.column_values.push_back(info->values[j]->data());
                j++;
                continue;
            }
            roh.column_values.push_back(tab_ptr->column_datas[i].default_value);
        }
    } else {
        for (int i = 0; i < tab_ptr->column_datas.size(); i++) {
            if (j < info->values.size()) {
                roh.column_values.push_back(info->values[j]->data());
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
        eval_push_error_return("eval_insert_into(): werid bug");}

    if (!errors.empty()) {
        return;}

    tab_ptr->rows.push_back(roh);
}
