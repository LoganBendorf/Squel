#include "pch.h"

#include "helpers.h"
#include "allocators.h"
#include "allocator_aliases.h"
#include "structs.h"
#include "test_reader.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "print.h"
#include "object.h"
#include "node.h"
#include "execute.h"
#include "files.h"
#include "logger.h"

#define SOCKET_PATH "/tmp/squel_daemon.sock"

logger<error_msg> errors{};
logger<error_msg> sql_errors{};
logger<warning_msg> sql_warnings{};

avec<table_cache> table_caches;
std::vector<SP<e_function_object>> g_functions;

std::string input;

display_table display_tab = {false, nullptr};

cmd_line_arguments g_args;

static std::vector<test_container> tests;



bool DEBUG = true;
bool TIME = false;
bool IS_DAEMON = false;


struct qt_display {
    QGridLayout* commands_results_label;
    QGridLayout* table_grid;
};

struct daemon_cmd_line_arguments {
    std::string query;

    bool has_save_loc;
    std::string save_loc;

    bool debug;
    bool time;
};


static cmd_line_arguments parse_cmd_line_args(std::vector<std::string> args);
static std::expected<daemon_cmd_line_arguments, std::string> parse_daemon_cmd_line_args(const std::string& arg_str);

static int run_as_daemon(const cmd_line_arguments& args);
static int cmd_line_run(const cmd_line_arguments& args, qt_display* qt_info);
static int cmd_line_run_tests(const cmd_line_arguments& args);
static int visual_run([[maybe_unused]] cmd_line_arguments args);


static void display_errors(QGridLayout* commands_results_label);
static void display_graphical_table(QGridLayout* table_grid);

static auto clear_layout = [](QLayout* layout) {
    QLayoutItem* item = nullptr;
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



static cmd_line_arguments parse_cmd_line_args(std::vector<std::string> args) {

    cmd_line_arguments ret_val {
                         false
                        ,false
                        ,""
                        ,false
                        ,""
                        ,false
                        , false
                        , false};

    for (size_t i = 0; i < args.size(); i++) {
        std::string arg = std::string(args[i]);

        if (arg.size() == 0) {
            throw std::runtime_error("Zero sized flag somehow"); }

        if (arg == "-t") {
            ret_val.run_tests = true;
        } else if (arg == "-b") {
            if (IS_DAEMON) {
                throw std::runtime_error("Query sent to daemon tried to run as a daemon"); }
            ret_val.background = true;
        } else if (arg == "-d") {
            ret_val.debug = true;
        } else if (arg == "-p") { // -p for profile smile
            ret_val.time = true;
        } else if (arg == "-q") {
            if (i == args.size() - 1) {
                const char* val = std::getenv("SQUEL_QUERY");
                if (val == nullptr) {
                    throw std::runtime_error("'-q' flag set but no query exists in args or env"); }

                ret_val.query = std::string(val);
                ret_val.has_query = true;
                continue;
            }
            const std::string potential_query = std::string(args[i + 1]);
            if (potential_query.size() > 0 && potential_query[0] == '-') { // Use env
                const char* val = std::getenv("SQUEL_QUERY");
                if (val == nullptr) {
                    throw std::runtime_error("'-q' flag set but no query exists in args or env"); }

                ret_val.query = std::string(val);
                ret_val.has_query = true;
                continue;
            }
            ret_val.query = args[i + 1];
            ret_val.has_query = true;
            i++;
        } else if (arg == "-s") {
            if (i >= args.size() - 1) {
                throw std::runtime_error("No path after save flag"); }

            ret_val.save_loc = args[i + 1];
            ret_val.has_save_loc = true;
            i++;
        } else {
            throw std::runtime_error("Unknown flag \"" + arg + "\"");
        }
    }

    return ret_val;
}

// Must have -q flag or fail
static std::expected<daemon_cmd_line_arguments, std::string> parse_daemon_cmd_line_args(const std::string& arg_str) {

    daemon_cmd_line_arguments ret_val {
                        .query        = "",
                        .has_save_loc = false,
                        .save_loc     = "",
                        .debug        = false,
                        .time         = false };

    bool has_at_least_one_flag = false;
    size_t input_pos = 0;
    while (input_pos < arg_str.size()) {
        const char charac = arg_str[input_pos];
        if (std::isspace(charac)) {
            input_pos++;
            continue;
        }

        bool should_break = false;
        if (charac == '-') {
            if (input_pos + 1 < arg_str.length()) {
                input_pos++;
            } else {
                return std::unexpected("Malformed cmd line args, contained '-' but no flag character after"); }
            
            const char flag = arg_str[input_pos];
            switch (flag) {
            case 'q': {
                if (input_pos + 2 >= arg_str.length()) {
                    return std::unexpected("Cmd line args contained '-q' but no query afterwards"); }
                
                if (' ' != (arg_str[input_pos + 1])) {
                    return std::unexpected("No space after '-q'"); }                

                input_pos += 2;

                ret_val.query = arg_str.substr(input_pos, arg_str.length() - 1); 
                should_break = true;

            } break;
            case 's': {
                if (input_pos + 2 >= arg_str.length()) {
                    return std::unexpected("Cmd line args contained '-q' but no save location afterwards"); }
                
                if (' ' != (arg_str[input_pos + 1])) {
                    return std::unexpected("No space after '-q'"); }                

                input_pos += 2;

                size_t save_loc_start = input_pos;
                while (input_pos < arg_str.size() && !std::isspace(arg_str[input_pos])) {
                    input_pos++; }

                ret_val.has_save_loc = true;
                ret_val.save_loc = arg_str.substr(save_loc_start, input_pos);

            } break;
            case 'd': ret_val.debug = true; input_pos++; break;
            case 'p': ret_val.time = true;  input_pos++; break;
            default: return std::unexpected("Unknown flag, '-" + std::string(1, flag) + "'"); }
        } else {
            return std::unexpected("Malformed cmd line args" + std::string(!has_at_least_one_flag ? ", forgot flags?" : "")); 
        }

        has_at_least_one_flag = true;

        if (should_break) {
            break; 
        }

    }

    if (ret_val.query == "") {
        return std::unexpected("Did not contain query"); 
    }

    return ret_val;
}

static int cmd_line_run(const cmd_line_arguments& args, qt_display* qt_info) {

    DEBUG = false;

    if (args.debug) {
        DEBUG = true; }

    if (args.time) {
        TIME = true; }

    if (args.has_query) {
        input = args.query; }

    if (input.length() > 2000) {
        std::cerr << "INPUT TOO LONG >:(\n"; return 1;}

    if (input.length() == 0) {
        std::cerr << "No input\n"; return 1;}

    

    if (DEBUG) [[unlikely]] {
        std::cout << "PRINTING INPUT -----------------\n";
        std::cout << "'" << input << "'\n";
        std::cout << "DONE ---------------------------\n\n";
    }



    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> file_start;
    if (TIME)  [[unlikely]] { file_start = std::chrono::high_resolution_clock::now(); }

    // bool rc = false;
    // if (args.has_save_loc) {
    //     rc = read_saved_files(g_tables, args.save_loc);
    // } else {
    //     rc = read_saved_files(g_tables); }
    // if (!rc) {
    //     FATAL_ERROR_THROW("Failed to read tables", CUR_LOC); }

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> file_end;
    if (TIME)  [[unlikely]] { file_end = std::chrono::high_resolution_clock::now(); }



    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> allocate_start;
    if (TIME)  [[unlikely]] { allocate_start = std::chrono::high_resolution_clock::now(); }

    constexpr size_t size = 1 << 20;
    main_alloc<void>::allocate_stack_memory(size);

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> allocate_end;
    if (TIME)  [[unlikely]] { allocate_end = std::chrono::high_resolution_clock::now(); }



    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> query_start;
    if (TIME)  [[unlikely]] { query_start = std::chrono::high_resolution_clock::now(); }

    try {

    std::vector<token> tokens = lexer(input);
    if (DEBUG) [[unlikely]] { print_tokens(tokens); }

    parser_init(tokens);
    avec<UP<node>> nodes = parse();
    if (DEBUG) [[unlikely]] { print_nodes(nodes); }

    eval_init(std::move(nodes));
    nodes.clear();
    avec<UP<e_node>> e_nodes = eval();
    if (DEBUG) [[unlikely]] { print_e_nodes(e_nodes); }

    execute_init(std::move(e_nodes));
    execute();
    e_nodes.clear();

    } catch (const std::runtime_error& e) {
        std::cerr << "Query failed. " << e.what() << std::endl;
        std::cerr << sql_errors.get_formated_msgs();
        sql_errors.clear_log();
        return(1);
    }

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> query_end;
    if (TIME) [[unlikely]] { query_end = std::chrono::high_resolution_clock::now(); }


    // Buggy with persistent objects
    // main_alloc<void>::deallocate_stack_memory();

    // const auto mem_used = static_cast<char*>(main_alloc<void>::stack.top) - static_cast<char*>(main_alloc<void>::stack.base);
    // std::cout << "Arena bytes used = " << mem_used << "\n";


    const bool is_graphical = !args.has_query && !args.run_tests;

    if (sql_errors.has_msgs()) {
        std::cerr << sql_errors.get_formated_msgs();

        if (is_graphical) {
            display_errors(qt_info->commands_results_label); }

    } else if (is_graphical) {
        clear_layout(qt_info->commands_results_label);
        QLabel* results_label = new QLabel("No errors");
        qt_info->commands_results_label->addWidget(results_label, 0, 0);

        if (display_tab.to_display) {
            display_graphical_table(qt_info->table_grid); }

        if (errors.has_msgs()) {
            std::cerr << errors.get_formated_msgs();
            errors.clear_log();
        }

    }

    if (DEBUG) [[unlikely]] {
        if (sql_warnings.has_msgs()) {
            std::cout << "WARNINGS ----------------\n";
            std::cout << sql_warnings.get_formated_msgs();
            std::cout << "DONE ---------------------------\n\n";
        }
        sql_warnings.clear_log();
    }

    
    if (sql_errors.has_msgs()) {
        sql_errors.clear_log();
        return 1; 
    } 



    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> save_start;
    if (TIME)  [[unlikely]] { save_start = std::chrono::high_resolution_clock::now(); }

    // if (args.has_save_loc) {
    //     rc = save_tables(g_tables, args.save_loc);
    // } else {
    //     rc = save_tables(g_tables); }
    // if (!rc) {
    //     FATAL_ERROR_THROW("Failed to save tables", CUR_LOC); }

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> save_end;
    if (TIME) [[unlikely]] { save_end = std::chrono::high_resolution_clock::now(); }

    if (TIME) [[unlikely]] {
        std::chrono::duration<double> file_elapsed = (*file_end) - (*file_start);
        std::cout << "File read time:         " << file_elapsed.count() * 1000 << " miliseconds" << std::endl;
        
        std::chrono::duration<double> allocate_elapsed = (*allocate_end) - (*allocate_start);
        std::cout << "Memory allocation time: " << allocate_elapsed.count() * 1000 << " miliseconds"  << std::endl;

        std::chrono::duration<double> query_elapsed = (*query_end) - (*query_start);
        std::cout << "Query time:             " << query_elapsed.count() * 1000 << " miliseconds"  << std::endl;

        std::chrono::duration<double> save_elapsed = (*save_end) - (*save_start);
        std::cout << "File save time:         " << save_elapsed.count() * 1000 << " miliseconds"  << std::endl;
    }

    return 0;
}



static int cmd_line_run_tests(const cmd_line_arguments& args) {

    tests = init_read_test();

    // Alphabetical sort
    std::ranges::sort(tests, [](const test_container& a, const test_container& b) {
        return a.folder_name < b.folder_name;
    });

    for (const auto& test : tests) {
        for (size_t j = 0; j < test.test_paths.size(); j++) {

            const std::string full_path = test.test_paths[j];
            const std::string path = full_path.substr(test.folder_name.length() + 1, full_path.length());
            
            const size_t test_index = j;
            const auto& [test_input, expect_fail] = read_test(test, test_index);
            input = test_input;

            constexpr size_t line_size = 30;

            const std::string path_str = path + (expect_fail ? "expect fail" : "");
            
            const size_t pad_size = (line_size > path_str.size()) ?
                                        line_size - path_str.size() : 0;
            
            std::cout << path_str << std::string(pad_size, '-') << std::endl;

            int rc = 1;
            try {
                rc = cmd_line_run(args, nullptr);
            } catch (const std::runtime_error& e) {
                std::cout << "Exception: " << e.what() << "\n";
                rc = 1;
            } catch (const std::exception& e) {
                std::cout << "Exception: " << e.what() << "\n";
                rc = 1;
            }

            constexpr size_t test_result_size = 9;
            if (expect_fail && rc == 1) {
                std::cout << GREEN << "TEST PASS" << RESET << std::string(line_size - test_result_size, '-') << std::endl;
            } else if (expect_fail) {
                std::cout << RED   << "TEST FAIL" << RESET << std::string(line_size - test_result_size, '-') << std::endl;
            }

            if (!expect_fail && rc == 0) {
                std::cout << GREEN << "TEST PASS" << RESET << std::string(line_size - test_result_size, '-') << std::endl;
            } else if (!expect_fail) {
                std::cout << RED   << "TEST FAIL" << RESET << std::string(line_size - test_result_size, '-') << std::endl;
            }
        }
    }

    return 0;
}

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int run_as_daemon(const cmd_line_arguments& args) {

    if (args.debug) {
        DEBUG = true; }

    int server_fd = -1;
    int client_fd = -1;
    struct sockaddr_un server_addr{};
    std::array<char, 1024> buffer{};
    
    // Create socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        FATAL_ERROR_STACK_TRACE_EXIT("Squel as daemon, socket creation failed", CUR_LOC); }
    
    // Setup address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);
    
    // Remove existing socket
    unlink(SOCKET_PATH);
    
    // Bind and listen
    auto* addr_ptr = reinterpret_cast<struct sockaddr*>(&server_addr);
    if (-1 == bind(server_fd, addr_ptr, sizeof(server_addr))) {
        FATAL_ERROR_STACK_TRACE_EXIT("Squel as daemon, bind failed", CUR_LOC); 
    }

    if (-1 == listen(server_fd, 5)) {
        perror("listen failed");
        close(server_fd);
        FATAL_ERROR_STACK_TRACE_EXIT("Squel as daemon, listen failed", CUR_LOC);
    }
    
    std::cout << "Daemon listening on " << SOCKET_PATH << "..." << std::endl;

    std::streambuf* real_cout = std::cout.rdbuf();
    
    while (true) {
        client_fd = accept(server_fd, nullptr, nullptr); // Blocks

        std::cout << "Daemon got a connection (" << client_fd << ")" << std::endl;
        
        ssize_t bytes_with_error = read(client_fd, buffer.data(), sizeof(buffer));
        if (bytes_with_error == -1) {
            const std::string& err_str = "ERROR: Read failed";
            write(client_fd, err_str.c_str(), err_str.length());
            shutdown(client_fd, SHUT_WR); 
            close(client_fd);
            continue;
        }

        size_t bytes = static_cast<size_t>(bytes_with_error);

        if (bytes < buffer.size()) {
            buffer[bytes] = '\0';   // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        } else {
            buffer[buffer.size() - 1] = '\0';
        }

        input = std::string{buffer.data()};
        std::cout << "Daemon received (" << input << ")" << std::endl;
        if (input.size() == 0) {
            const std::string& err_str = "ERROR: No input";
            write(client_fd, err_str.c_str(), err_str.length());
            shutdown(client_fd, SHUT_WR); 
            close(client_fd);
            continue;
        }

        std::cout << "Daemon parsing arguments..." << std::endl;

        auto daemon_args_result = parse_daemon_cmd_line_args(input);
        if (!daemon_args_result.has_value()) {
            const std::string& err_str = daemon_args_result.error();
            write(client_fd, err_str.c_str(), err_str.length());
            shutdown(client_fd, SHUT_WR); 
            close(client_fd);
            continue;
        }

        auto daemon_args = *daemon_args_result;

        auto query_args = cmd_line_arguments{
                            .run_tests    = false,
                            .has_query    = true,
                            .query        = daemon_args.query,
                            .has_save_loc = daemon_args.has_save_loc,
                            .save_loc     = daemon_args.save_loc,
                            .debug        = daemon_args.debug,
                            .time         = daemon_args.time,
                            .background   = false
                        };

        std::cout << "Daemon has parsed arguments" << std::endl;

        std::cout << "Daemon parsing query..." << std::endl;

        // Capture the output instead of printing to stdout
        std::ostringstream output_stream;
        // std::cout.rdbuf(output_stream.rdbuf());
        
        // Capture stderr too, this yoinks exceptions too so just use logger
        // std::ostringstream error_stream;
        // std::streambuf* real_cerr = std::cerr.rdbuf();
        // std::cerr.rdbuf(error_stream.rdbuf());
        
        int rc = 1;
        try {
            rc = cmd_line_run(query_args, nullptr);
        } catch (const std::runtime_error& e) {
            std::cerr << "Exception: " << e.what() << "\n";
            rc = 1;
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
            rc = 1;
        }
        
        // Restore streams
        std::cout.rdbuf(real_cout);

        // Send response back to client
        std::string response;
        if (rc == 0) {
            response   = "SUCCESS:" + output_stream.str();
            std::cout << "Query executed successfully '" << input << "'." << std::endl;
            std::cout << "With response '" << response << "'." << std::endl;
        } else {
            response   = "ERROR:" + output_stream.str();
            std::cout << "Query failed to execute '" << input << "'." << std::endl;
            std::cout << "With response '" << response << "'." << std::endl;
        }
        
        write(client_fd, response.c_str(), response.length());
        shutdown(client_fd, SHUT_WR); 
        close(client_fd);
    }
    
    return 0;
}

int main (int argc, char* argv[]) {

    if (argc == 0) {
        std::cerr << "argc == 0 for some reason" << std::endl; exit(1); }

    std::vector<std::string> cmd_args;
    cmd_args.reserve(size_t(argc));
    for (size_t i = 1; i < size_t(argc); i++) {
        cmd_args.emplace_back(argv[i]);
    }

    try {
        g_args = parse_cmd_line_args(cmd_args);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl; exit(1);
    }

    if (g_args.has_query && g_args.run_tests) {
        std::cerr << "Can not run test and cmd line query. Pick one" << std::endl; exit(1); }
    
    if (g_args.debug && g_args.time) {
        std::cerr << "'-d' and '-t' flags can not both be set" << std::endl; exit(1); }
    
    if (!g_args.has_query && !g_args.run_tests && !g_args.background) { // Not sure how to run tests and visuals, ig don't for now.
        std::cout << "QT GUI is enabled" << std::endl;
        std::cout << "Cmd line query: " << g_args.query << std::endl; 
        std::cout << "Test mode: " << (g_args.run_tests ? "enabled" : "disabled") << std::endl;
    } else {
        // std::cout << "QT GUI is disabled" << std::endl;
        // std::cout << "Cmd line query: " << g_args.query << std::endl; 
        // std::cout << "Test mode: " << (g_args.run_tests ? "enabled" : "disabled") << std::endl;
    }

    if (g_args.has_query && g_args.background) {
        std::cerr << "Cannot give query to daemon at startup. -q and -b are mutually exclusive" << std::endl; exit(1); }

    if (g_args.run_tests && g_args.background) {
        std::cerr << "Daemon can not run tests. -t and -b are mutually exclusive" << std::endl; exit(1); }

    if (g_args.background) {
        return run_as_daemon(g_args);
    }

    if (g_args.has_query) {
        return cmd_line_run(g_args, nullptr);
    }

    if (g_args.run_tests) {
        return cmd_line_run_tests(g_args);
    }
    
    return visual_run(g_args);
}

static int visual_run(cmd_line_arguments args) {
    // At the start of your main() function:
    QLoggingCategory::setFilterRules("qt.qpa.events.reader.debug=false");

    display_tab.to_display = false;

    //qt
    // QApplication app(/*argc, argv*/);
    int argc = 1;
    std::string app_name = "Squel";
    char* argv[] = {app_name.data()};   // NOLINT(cppcoreguidelines-avoid-c-arrays)
    QApplication app{argc, argv};       // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

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
    if (args.run_tests) {
        scroll_area_container->addWidget(init_test_button); }

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


    

    QVBoxLayout* test_info_layout = nullptr;
    QLabel* current_test_marker_label = nullptr;
    QLabel* current_test_label = nullptr;
    QFrame* test_frame = new QFrame();
    if (args.run_tests) {
        test_info_layout = new QVBoxLayout();
        current_test_marker_label = new QLabel("Current test:");
        current_test_label = new QLabel();

        test_info_layout->addWidget(current_test_marker_label);
        test_info_layout->addWidget(current_test_label);

        test_frame->setStyleSheet("background-color: #007ACC; color: white; font: 12pt 'Arial'; padding: 10px; border: 2px solid white; border-radius: 20px;");
        test_frame->setLayout(test_info_layout);

        layout->addWidget(test_frame);
    }

    static QGridLayout* commands_results_label = new QGridLayout();
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
        table_caches.clear();
        // Maybe should also clear the display table?
        std::cout << "Tables cleared" << std::endl;
    });


    // table_grid->itemAt(0);

    // Folding finding in tests isn't recursive, so no triple folders rn

    QObject::connect(init_test_button, &QPushButton::clicked, [&]() {\
        tests = init_read_test();

        // Alphabetical sort
        std::ranges::sort(tests, [](const test_container& a, const test_container& b) {
            return a.folder_name < b.folder_name;
        });
    
        scroll_area_container->removeWidget(init_test_button);
        init_test_button->hide();
        init_test_button->deleteLater();
        

        // FOLDER BUTTONS
        for (const auto& test : tests) {
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

            for (size_t j = 0; j < test.test_paths.size(); j++) {
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
                    const auto& [test_input, expect_fail] = read_test(test, test_index);
                    input = test_input;

                    current_test_label->setText(QString::fromStdString(input));

                    qt_display qt_info = {commands_results_label, table_grid};

                    cmd_line_run(g_args, &qt_info);

                });
            }
        }
    });
    

    QObject::connect(submit_button, &QPushButton::clicked, [&]() {

        // Input
        QString qt_input = input_text_edit->toPlainText();
        input = qt_input.toUtf8().constData();


        qt_display qt_info = {commands_results_label, table_grid};

        cmd_line_run(g_args, &qt_info);

        // Clean up for next loop
        display_tab.to_display = false;

    });

    window.show();

    auto qt_return = app.exec();

    for (size_t i = 0; i < table_caches.size(); i++) {
        if (table_caches[i].table.get() == nullptr) {
            std::cout << "g_tables[" << i << "] was null" << std::endl; }
    }

    table_caches.clear();
    g_functions.clear();

    return qt_return;
}

static void display_errors(QGridLayout* commands_results_label) {

    display_tab.to_display = false;
    clear_layout(commands_results_label);
    QLabel* error_label = new QLabel("Errors:");
    commands_results_label->addWidget(error_label, 0, 0);

    int x = 0;
    int y = 1;
    for (const auto& err_msg : sql_errors.get_formated_msgs_as_vec()) {
        QLabel* results_label = new QLabel(QString::fromStdString(err_msg));
        commands_results_label->addWidget(results_label, y, x);
        y++;
    }

    sql_errors.clear_log();
}

static void display_graphical_table(QGridLayout* table_grid) {
    clear_layout(table_grid);  
                        
    const SP<f_table_info_object>& tab_info = display_tab.table_info;

    const SP<table_object>& tab = tab_info->table;

    for (const auto& detail : tab->column_data) {
        if (detail == nullptr) {
            std::cout << "bruh, " << std::source_location::current().line() << std::endl; }
    }

    table_grid->addWidget(new QLabel(QString::fromStdString(std::string("Table: " + tab->table_name))), 0, 0); // err

    if (tab_info->col_ids.size() == 0) {
        return;}

    // new begin
    int y = 1;
    int x = 0;
    for (const auto& col_id : tab_info->col_ids) {

        const auto& [col_name, col_in_bounds] = tab->get_column_name(col_id);
        if (!col_in_bounds) {
            errors.add_msg("Out of bounds column index", CUR_LOC); return; }

        table_grid->addWidget(new QLabel(QString::fromStdString(std::string(col_name))), y, x);
        x++;
    }

    x = 0;
    y = 2;
    for (const auto& row_index : tab_info->row_ids) {

        auto result = tab->get_row_vec_ptr(row_index);
        if (!result.has_value()) {
            errors.add_msg("Out of bounds row index", CUR_LOC); return; }

        const auto& row = **result;

        for (const auto& col_id : tab_info->col_ids) {
            if (col_id >= row.size()) {
                errors.add_msg("Out of bounds column index", CUR_LOC); 
                return;
            }

            std::string cell_value = std::string(row[col_id]->data()); 
            if (row[col_id]->type() == NULL_OBJ) {
                cell_value = ""; }

            table_grid->addWidget(new QLabel(QString::fromStdString(cell_value)), y, x);
            x++;
        }
        y++;
        x = 0;
    }
}