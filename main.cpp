

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstring>
#include <cctype>

#include "structs_and_macros.h"
#include "test_reader.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

std::vector<std::string> errors;

std::vector<table> tables;

std::string input;

display_table display_tab;

// CURRENTLY WORKING ON:
// CREATE TABLE
// SELECT
// INSERT <-------------------------------- DOESNT WORK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!?

extern std::string keyword_enum_to_string[];

void print_column(table tab, int column_index) {

    column_data col = tab.column_datas[column_index];

    if (col.field_name.length() > 16) {
        std::cout << "nah field name is too long to print";
        exit(1);}

    std::cout << col.field_name << std::endl;

    for (int i = 0; i < tab.rows.size(); i++) {
        std:: cout << tab.rows[column_index].column_values[i] << std::endl;
    }
}


void print_tokens(std::vector<token> tokens) {
    std::cout << "PRINTING TOKENS ----------------\n";
    for (int i = 0; i < tokens.size(); i++) {
        std::cout << " Keyword: " << std::setw(15) << std::left << keyword_enum_to_string[tokens[i].keyword] + ","
                                       << " Value: " << tokens[i].data << "\n";
    }
    std::cout << "DONE ---------------------------\n\n";
}

void print_nodes(std::vector<node*> nodes) {
    std:: cout << "PRINTING NODES -----------------\n";
    for (int i = 0; i < nodes.size(); i++) {
        std::cout << std::to_string(i + 1) << ":\n" << nodes[i]->inspect();
    }
    std::cout << "DONE ---------------------------\n\n";
}

enum input_style {
    VISUAL, TEST
};

enum input_style input_style = TEST;

int main (int argc, char* argv[]) {

    display_tab.to_display = false;

    //qt
    QApplication app(argc, argv);

    // icon
    app.setWindowIcon(QIcon("./icons/squel.ico"));


    QWidget window;
    window.setWindowTitle("Squel");

    QVBoxLayout* main_layout = new QVBoxLayout(&window);

    QVBoxLayout* layout = new QVBoxLayout();
    QLabel* enter_command_label = new QLabel("ENTER COMMAND:");
    QLineEdit* input_line_edit = new QLineEdit();
    QPushButton* submit_button = new QPushButton("Submit");

    layout->addWidget(enter_command_label);
    layout->addWidget(input_line_edit);
    layout->addWidget(submit_button);

    QVBoxLayout* test_info_layout;
    QLabel* current_test_marker_label;
    QLabel* current_test_label;
    QPushButton* start_test_button;
    if (input_style == TEST) {
        test_info_layout = new QVBoxLayout();
        current_test_marker_label = new QLabel("Current test:");
        current_test_label = new QLabel();
        start_test_button = new QPushButton("start test");

        test_info_layout->addWidget(current_test_marker_label);
        test_info_layout->addWidget(current_test_label);
        test_info_layout->addWidget(start_test_button);

        main_layout->addLayout(test_info_layout);
    }

    main_layout->addLayout(layout);  // Input and submit

    QGridLayout* label_command_results = new QGridLayout();
    main_layout->addLayout(label_command_results);

    // Table grid
    QGridLayout* table_grid = new QGridLayout();
    main_layout->addLayout(table_grid);

    auto clear_layout = [](QLayout* layout) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget(); // deletes the widget
            delete item;           // deletes the layout item
        }
    };
    
    QObject::connect(start_test_button, &QPushButton::clicked, [&]() {\
        input = read_test();
        current_test_label->setText(QString::fromStdString(input));
        if (input.length() > 600) {
            std::cout << "INPUT TOO LONG >:(\n";
            return;}
        if (input.length() == 0) {
            std::cout << "No input\n";
            return;}

        std::cout << "PRINTING INPUT -----------------\n";
        std::cout << "'" << input << "'\n";
        std::cout << "DONE ---------------------------\n\n";

        std::vector<token> tokens = lexer(input);
        print_tokens(tokens);

        parser_init(tokens);
        std::vector<node*> nodes = parse();
        print_nodes(nodes);

        eval_init(nodes);
        eval();

        if (!errors.empty()) {
            //clear_layout(table_grid);
            clear_layout(label_command_results);
            QLabel* label_error = new QLabel("Errors:");
            label_command_results->addWidget(label_error, 0, 0);
            for (int i = 0; i < errors.size(); i++) {
                std::cout << "ERROR: " + errors[i] + "\n";
                QLabel* label_results = new QLabel(QString::fromStdString(errors[i]));
                int y = i + 1;
                int x = 0;
                label_command_results->addWidget(label_results, y, x);
            }
        } else {
            clear_layout(label_command_results);
            QLabel* label_results = new QLabel("No errors");
            label_command_results->addWidget(label_results, 0, 0);

            if (display_tab.to_display) {
                // Select items grid
                clear_layout(table_grid);  
            
                table tab = display_tab.tab;

                table_grid->addWidget(new QLabel(QString::fromStdString("Table: " + tab.name)));

                // Column names
                for (int i = 0; i < tab.column_datas.size(); i++) {
                    int y = 1; 
                    int x = i;  // can overflow
                    table_grid->addWidget(new QLabel(QString::fromStdString(tab.column_datas[i].field_name)), y, x);
                }

                // Row data
                for (int i = 1; i < tab.rows.size(); i++) {
                    for (int j = 0; j < tab.rows[0].column_values[0].size(); j++) {
                        int y = i + 1;
                        int x = j;
                        table_grid->addWidget(new QLabel(QString::fromStdString(tab.rows[i].column_values[j])), y, x);
                    }
                }
            }
        }

    });

    QObject::connect(submit_button, &QPushButton::clicked, [&]() {

        // Input
        QString qt_input = input_line_edit->text();
        input = qt_input.toUtf8().constData();


        if (input.length() > 600) {
            std::cout << "INPUT TOO LONG >:(\n";
            return;}
        if (input.length() == 0) {
            std::cout << "No input\n";
            return;}

        std::vector<token> tokens = lexer(input);
        print_tokens(tokens);
        
        parser_init(tokens);
        std::vector<node*> nodes = parse();
        print_nodes(nodes);
        
        eval_init(nodes);
        eval();

        if (!errors.empty()) {
            //clear_layout(table_grid);
            clear_layout(label_command_results);
            QLabel* label_error = new QLabel("Errors:");
            label_command_results->addWidget(label_error, 0, 0);
            for (int i = 0; i < errors.size(); i++) {
                std::cout << "ERROR: " + errors[i] + "\n";
                QLabel* label_results = new QLabel(QString::fromStdString(errors[i]));
                int y = i + 1;
                int x = 0;
                label_command_results->addWidget(label_results, y, x);
            }
        } else {
            clear_layout(label_command_results);
            QLabel* label_results = new QLabel("No errors");
            label_command_results->addWidget(label_results, 0, 0);

           if (display_tab.to_display) {
                // Select items grid
                clear_layout(table_grid);  
            
                table tab = display_tab.tab;

                table_grid->addWidget(new QLabel(QString::fromStdString("Table: " + tab.name)));

                // Column names
                for (int i = 0; i < tab.column_datas.size(); i++) {
                    int y = 1; 
                    int x = i;  // can overflow
                    table_grid->addWidget(new QLabel(QString::fromStdString(tab.column_datas[i].field_name)), y, x);
                }

                // Row data
                for (int i = 1; i < tab.rows.size(); i++) {
                    for (int j = 0; j < tab.rows[0].column_values[0].size(); j++) {
                        int y = i + 1;
                        int x = j;
                        table_grid->addWidget(new QLabel(QString::fromStdString(tab.rows[i].column_values[j])), y, x);
                    }
                }
            }
        }

        // Clean up for next loop
        display_tab.to_display = false;
        input_line_edit->clear();
        errors.clear();
        tokens.clear();

    });

    window.show();

    return app.exec();
}