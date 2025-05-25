
#include "pch.h"

#include "structs_and_macros.h"
#include "test_reader.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "print.h"
#include "arena.h"
#include "object.h"

std::vector<std::string> errors;
std::vector<std::string> warnings;

static std::vector<table_object*> g_tables;
static std::vector<evaluated_function_object*> g_functions;

std::string input = "";

display_table display_tab = {false, nullptr};

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

static auto toggle_show_test_children = [](QLayout* layout) {
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

static void clear_g_tables() {
    for (const auto& tab : g_tables) {
        delete tab;
    }
    g_tables.clear();
}

static void clear_g_functions() {
    for (const auto& func : g_functions) {
        delete func;
    }
    g_functions.clear();
}

enum input_style {
    VISUAL, TEST
};

constexpr enum input_style input_style = TEST;

int main (int argc, char* argv[]) {

    // arena_inst.construct();

    display_tab.to_display = false;

    //qt
    QApplication app(argc, argv);

    // icon
    app.setWindowIcon(QIcon("./icons/squel.ico"));


    QWidget window;
    window.setWindowTitle("Squel");

    QVBoxLayout* main_layout = new QVBoxLayout(&window);

    QWidget* const fat_layout_container = new QWidget();
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
        g_tables.clear();
        // Maybe should also clear the display table?
        std::cout << "Tables cleared" << std::endl;
    });


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
        for (size_t i = 0; i < tests.size(); i++) {
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

            for (size_t j = 0; j < tests[i].test_paths.size(); j++) {
                std::string path = test.test_paths[j];
                path = path.substr(test.folder_name.length() + 1, path.length());
                std::string button_name = path + ": Start test";

                QPushButton* test_start_button = new QPushButton(button_name.c_str());
                test_layout->addWidget(test_start_button);
                test_start_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
                test_start_button->setStyleSheet("text-align: left; padding-left: 15px;");

                test_start_button->hide();
                
                size_t test_index = j;

                QObject::connect(test_start_button, &QPushButton::clicked, [&, test, test_index]() {\
                    input = read_test(test, test_index);
                    current_test_label->setText(QString::fromStdString(input));

                    if (input.length() > 2000) {
                        std::cout << "INPUT TOO LONG >:(\n";
                        return;}

                    if (input.length() == 0) {
                        std::cout << "No input\n";
                        return;}
            
                    std::cout << "PRINTING INPUT -----------------\n";
                    std::cout << "'" << input << "'\n";
                    std::cout << "DONE ---------------------------\n\n";
                        
                    constexpr size_t size = 1 << 18;
                    std::byte stack_buffer[size];
                    arena_inst.init(stack_buffer, size);

                    auto start = std::chrono::high_resolution_clock::now();

                    std::vector<token> tokens = lexer(input);
                    // print_tokens(tokens);

                    parser_init(tokens, g_functions, g_tables);
                    std::vector<node*> nodes = parse();
                    // print_nodes(nodes);

                    environment* env = eval_init(nodes, g_functions, g_tables);
                    const auto& [funcs, tables] = eval(env);
                    clear_g_functions();
                    for (const auto& new_func : funcs) {
                        g_functions.push_back(new_func->clone(HEAP)); 
                    }
                    clear_g_tables();
                    for (const auto& tab : tables) {
                        g_tables.push_back(tab->clone(HEAP));
                    }

                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = end - start;
                    std::cout << "Elapsed time: " << elapsed.count() * 1000 << " miliseconds\n";

                    std::cout << "Arena bytes used = " << arena_inst.get_usage() << "\n";

                    arena_inst.destroy();
            
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
                        for (const auto& warning : warnings) {
                            std::cout << "WARNING: " << warning << std::endl;
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

        arena_inst.init();

        std::vector<token> tokens = lexer(input);
        print_tokens(tokens);

        parser_init(tokens, g_functions, g_tables);
        std::vector<node*> nodes = parse();
        print_nodes(nodes);

        environment* env = eval_init(nodes, g_functions, g_tables);
        const auto& [funcs, tables] = eval(env);
        clear_g_functions();
        for (const auto& new_func : funcs) {
            g_functions.push_back(new_func->clone(HEAP)); 
        }
        clear_g_tables();
        for (const auto& tab : tables) {
            g_tables.push_back(tab->clone(HEAP));
        }

        std::cout << "Arena bytes used = " << arena_inst.get_usage() << "\n";

        arena_inst.destroy();

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
            for (const auto& warning : warnings) {
                std::cout << "WARNING: " << warning << std::endl;
            }
            std::cout << "DONE ---------------------------\n\n";
        }
        warnings.clear();

        // Clean up for next loop
        display_tab.to_display = false;

    });

    window.show();

    
    auto qt_return = app.exec();

    clear_g_functions();
    clear_g_tables();

    // arena_inst.teardown();

    return qt_return;
}

static void display_errors(QGridLayout* commands_results_label) {
    display_tab.to_display = false;
    clear_layout(commands_results_label);
    QLabel* error_label = new QLabel("Errors:");
    commands_results_label->addWidget(error_label, 0, 0);

    int x = 0;
    int y = 1;
    for (const auto& error : errors) {
        std::cout << "ERROR: " + error + "\n";
        QLabel* results_label = new QLabel(QString::fromStdString(error));
        commands_results_label->addWidget(results_label, y, x);
        y++;
    }

    errors.clear();
}

static void display_graphical_table(QGridLayout* table_grid) {
    clear_layout(table_grid);  
                        
    table_info_object* tab_info = display_tab.table_info;

    table_object* tab = tab_info->tab;

    table_grid->addWidget(new QLabel(QString::fromStdString("Table: " + *tab->table_name)), 0, 0); // err

    if (tab_info->col_ids->size() == 0) {
        return;}

    // new begin
    int y = 1;
    int x = 0;
    for (const auto& col_id : *tab_info->col_ids) {

        const auto& [col_name, col_in_bounds] = tab->get_column_name(col_id);
        if (!col_in_bounds) {
            errors.push_back("display_graphical_table(): Out of bounds column index"); return; }

        table_grid->addWidget(new QLabel(QString::fromStdString(col_name)), y, x);
        x++;
    }

    x = 0;
    y = 2;
    for (const auto& row_index : *tab_info->row_ids) {

        const auto& [row, row_index_in_bounds] = tab->get_row_vector(row_index);
        if (!row_index_in_bounds) {
            errors.push_back("print_table(): Out of bounds row index"); return; }
        for (const auto& col_id : *tab_info->col_ids) {
            if (col_id >= row.size()) {
                errors.push_back("print_table(): Out of bounds column index"); 
                return;
            }

            std::string cell_value = row[col_id]->data(); 
            if (row[col_id]->type() == NULL_OBJ) {
                cell_value = ""; }

            table_grid->addWidget(new QLabel(QString::fromStdString(cell_value)), y, x);
            x++;
        }
        y++;
        x = 0;
    }

}