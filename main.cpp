
#include "pch.h"


#include "structs_and_macros.h"
#include "test_reader.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "print.h"

std::vector<std::string> errors;
std::vector<std::string> warnings;

std::vector<table> tables;

std::string input;

display_table display_tab;

std::vector<struct test> tests;

// CURRENTLY WORKING ON:
// CREATE TABLE
// SELECT
// INSERT -----------> Working on types

static void display_errors(QGridLayout* commands_results_label);
static void display_graphical_table(QGridLayout* table_grid);

static auto clear_layout = [](QLayout* layout) {
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget(); // deletes the widget
        delete item;           // deletes the layout item
    }
};


enum input_style {
    VISUAL, TEST
};

constexpr enum input_style input_style = TEST;

int main (int argc, char* argv[]) {

    display_tab.to_display = false;

    //qt
    QApplication app(argc, argv);

    // icon
    app.setWindowIcon(QIcon("./icons/squel.ico"));


    QWidget window;
    window.setWindowTitle("Squel");

    QVBoxLayout* main_layout = new QVBoxLayout(&window);

    QWidget* fat_layout_container = new QWidget();
    main_layout->addWidget(fat_layout_container);

    QHBoxLayout* fat_layout = new QHBoxLayout(fat_layout_container);
    fat_layout_container->setLayout(fat_layout);
    
    QVBoxLayout* scroll_area_container = new QVBoxLayout();
    fat_layout->addLayout(scroll_area_container);
    QPushButton* init_test_button = new QPushButton("init tests");
    if (input_style == TEST) {
        scroll_area_container->addWidget(init_test_button);
    }
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true); 
    scroll_area_container->addWidget(scrollArea);
    QWidget* scroll_container = new QWidget();
    scrollArea->setWidget(scroll_container);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scroll_container);
    scroll_container->setLayout(scrollLayout);
    scrollLayout->setAlignment(Qt::AlignTop);
    scrollLayout->setSpacing(5); 

    // for (int i = 0; i < 5 ; i++) {
    //     scrollLayout->addWidget(new QLabel(QString("Label %1").arg(i + 1)));
    // }

    scroll_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll_container->setMinimumHeight(700);  // Adjust this based on content size
    scrollArea->setFixedHeight(600);
    scrollArea->setFixedWidth(400);
    fat_layout_container->setFixedHeight(600);

    QVBoxLayout* layout = new QVBoxLayout();
    fat_layout->addLayout(layout);  // Input and submit

    QLabel* enter_command_label = new QLabel("ENTER COMMAND:");
    layout->addWidget(enter_command_label);

    QTextEdit* input_text_edit = new QTextEdit();
    input_text_edit->setTabStopDistance(4 * QFontMetrics(input_text_edit->font()).horizontalAdvance(' ')); // Tab size
    layout->addWidget(input_text_edit);

    QPushButton* submit_button = new QPushButton("Submit");
    layout->addWidget(submit_button);


    

    QVBoxLayout* test_info_layout;
    QLabel* current_test_marker_label;
    QLabel* current_test_label;
    QFrame* test_frame = new QFrame();
    if (input_style == TEST) {
        test_info_layout = new QVBoxLayout();
        current_test_marker_label = new QLabel("Current test:");
        current_test_label = new QLabel();

        test_info_layout->addWidget(current_test_marker_label);
        test_info_layout->addWidget(current_test_label);

        test_frame->setStyleSheet("background-color: #007ACC; color: white; font: 12pt 'Arial'; padding: 10px; border: 2px solid white; border-radius: 20px;");
        test_frame->setLayout(test_info_layout);

        layout->addWidget(test_frame);
    }

    QGridLayout* commands_results_label = new QGridLayout();
    main_layout->addLayout(commands_results_label);

    // Table grid
    QFrame* table_frame = new QFrame();
    table_frame->setStyleSheet("background-color: #a8dadc; color:rgb(34, 55, 66); font: 14pt 'Arial'; padding: 5px; border: 2px solid white; border-radius: 4px;");
    QGridLayout* table_grid = new QGridLayout(table_frame);
    table_frame->setLayout(table_grid);
    main_layout->addWidget(table_frame);
        


    // Clear tables
    QPushButton* clear_tables_button = new QPushButton("Clear tables");
    clear_tables_button->setStyleSheet(
        "QPushButton { background-color:rgb(200, 81, 57); color: white; border: 2px solid rgb(52, 26, 21); border-radius: 10px; padding: 5px; }"
        "QPushButton:hover { background-color: rgb(171, 65, 44); }"
        "QPushButton:pressed { background-color: rgb(76, 29, 20); }" // This restores the press animation
    );    
    main_layout->insertWidget(0, clear_tables_button);
    QObject::connect(clear_tables_button, &QPushButton::clicked, [&] () {
        tables.clear();
        // Maybe should also clear the display table?
        std::cout << "Tables cleared" << std::endl;
    });


    auto toggle_show_test_children = [](QLayout* layout) {
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (item) {
                QWidget* widget = item->widget();
                if (widget) {
                    widget->setVisible(!widget->isVisible());
                }
            }
        }
    };

    // table_grid->itemAt(0);

    // Folding finding in tests isn't recursive, so no triple folders rn

    QObject::connect(init_test_button, &QPushButton::clicked, [&]() {\
        tests = init_read_test();

        // Alphabetical sort
        std::sort(tests.begin(), tests.end(), [](const test& a, const test& b) {
            return a.folder_name < b.folder_name;
        });
    
        scroll_area_container->removeWidget(init_test_button);
        init_test_button->hide();
        init_test_button->deleteLater();
        

        // FOLDER BUTTONS
        for (int i = 0; i < tests.size(); i++) {
            struct test test = tests[i];
            QPushButton* folder_show_button = new QPushButton(test.folder_name.c_str());
            folder_show_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
            scrollLayout->addWidget(folder_show_button);

            QVBoxLayout* test_layout = new QVBoxLayout();
            scrollLayout->addLayout(test_layout);

            test_layout->setContentsMargins(20, 0, 0, 0);  // Left, top, right, bottom margins


            QObject::connect(folder_show_button, &QPushButton::clicked, [&, test_layout] () {
                toggle_show_test_children(test_layout);
            });

            
            // INDIVIDUAL TESTS

            for (int j = 0; j < tests[i].test_paths.size(); j++) {
                std::string path = test.test_paths[j];
                path = path.substr(test.folder_name.length() + 1, path.length());
                std::string button_name = path + ": Start test";

                QPushButton* test_start_button = new QPushButton(button_name.c_str());
                test_layout->addWidget(test_start_button);
                test_start_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
                test_start_button->setStyleSheet("text-align: left; padding-left: 15px;");

                test_start_button->hide();
                
                int test_index = j;

                QObject::connect(test_start_button, &QPushButton::clicked, [&, test, test_index]() {\
                    input = read_test(test, test_index);
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
            
                    environment* env = eval_init(nodes);
                    eval(env);
            
                    if (!errors.empty()) {
                        display_errors(commands_results_label);
                    } else {
                        clear_layout(commands_results_label);
                        QLabel* results_label = new QLabel("No errors");
                        commands_results_label->addWidget(results_label, 0, 0);
            
                        if (display_tab.to_display) {
                            display_graphical_table(table_grid);
                        }
                    }

                    if (!warnings.empty()) {
                        std::cout << "WARNINGS ----------------\n";
                        for (int i = 0; i < warnings.size(); i++) {
                            std::cout << "WARNING: " << warnings[i] << std::endl;
                        }
                        std::cout << "DONE ---------------------------\n\n";
                    }
                    warnings.clear();

                });
            }
        }
    });
    

    QObject::connect(submit_button, &QPushButton::clicked, [&]() {

        // Input
        QString qt_input = input_text_edit->toPlainText();
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
        
        environment* env = eval_init(nodes);
        eval(env);

        if (!errors.empty()) {
            display_errors(commands_results_label);
        } else {
            clear_layout(commands_results_label);
            QLabel* results_label = new QLabel("No errors");
            commands_results_label->addWidget(results_label, 0, 0);

           if (display_tab.to_display) {
                display_graphical_table(table_grid);
            }
        }

        if (!warnings.empty()) {
            std::cout << "WARNINGS ----------------\n";
            for (int i = 0; i < warnings.size(); i++) {
                std::cout << "WARNING: " << warnings[i] << std::endl;
            }
            std::cout << "DONE ---------------------------\n\n";
        }
        warnings.clear();

        // Clean up for next loop
        display_tab.to_display = false;
        // input_text_edit->clear();
        errors.clear();
        tokens.clear();

    });

    window.show();

    return app.exec();
}

static void display_errors(QGridLayout* commands_results_label) {
    display_tab.to_display = false;
    clear_layout(commands_results_label);
    QLabel* error_label = new QLabel("Errors:");
    commands_results_label->addWidget(error_label, 0, 0);
    for (int i = 0; i < errors.size(); i++) {
        std::cout << "ERROR: " + errors[i] + "\n";
        QLabel* results_label = new QLabel(QString::fromStdString(errors[i]));
        int y = i + 1;
        int x = 0;
        commands_results_label->addWidget(results_label, y, x);
    }
    errors.clear();
}

static void display_graphical_table(QGridLayout* table_grid) {
    clear_layout(table_grid);  
                        
    table tab = display_tab.tab;

    table_grid->addWidget(new QLabel(QString::fromStdString("Table: " + tab.name)), 0, 0);

    if (display_tab.column_names.size() == 0) {
        return;}

    // Column names
    std::vector<int> row_indexes;
    int j = 0;
    for (int i = 0; i < display_tab.column_names.size(); i++) {
        while (tab.column_datas[j].field_name != display_tab.column_names[i]) {
            j++;}
        row_indexes.push_back(j);
        int y = 1;
        int x = i;  // can overflow
        table_grid->addWidget(new QLabel(QString::fromStdString(tab.column_datas[j].field_name)), y, x);
    }

    // for (row in rows) {
    //     for (index in row_indexes) {
    //         display row
    //     }
    // }
    for (int i = 0; i < display_tab.row_ids.size(); i++) {
        row tab_row = tab.rows[display_tab.row_ids[i]];
        for (int j = 0; j < row_indexes.size(); j++) {
            int y = i + 2;
            int x = j;
            table_grid->addWidget(new QLabel(QString::fromStdString(tab_row.column_values[j])), y, x);
        }
    }



    // // Row data
    // for (int i = 0; i < tab.rows.size(); i++) {
    //     for (int j = 0; j < tab.rows[0].column_values.size(); j++) {
    //         int y = i + 2;
    //         int x = j;
    //         table_grid->addWidget(new QLabel(QString::fromStdString(tab.rows[i].column_values[j])), y, x);
    //     }
    // }
}