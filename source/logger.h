
// TODO Make cpp file

#include "macros.h"
#include "token.h"

#include <vector>
#include <string>
#include <source_location>

constexpr std::string GREEN  = "\x1b[32m";
constexpr std::string YELLOW = "\x1b[33m";
constexpr std::string RED    = "\x1b[31m";
constexpr std::string RESET  = "\x1b[0m";

class message {
    public:
    [[nodiscard]] virtual std::string format(size_t loc_size, size_t longest_msg_size) const = 0;
    virtual ~message() noexcept = default;

    [[nodiscard]] virtual bool is_error_msg() const { return false; }

    std::string str;
};

class warning_msg : public message {
    public:
    explicit warning_msg(std::string msg) {
        str = msg;
    }
    
    [[nodiscard]] std::string format([[maybe_unused]] size_t loc_size, [[maybe_unused]] size_t longest_msg_size) const override {
        std::stringstream stream;
        stream << "WARNING: " << YELLOW << str << RESET;
        return stream.str();
    }
};

class error_msg : public message {
    public:
    error_msg() = default;
    explicit error_msg(std::string msg, const std::source_location& loc) {
        std::stringstream stream;
        stream << GET_ERROR_LOCATION(loc);
        err_loc_msg = stream.str();
        
        str = msg;
    }
    
    [[nodiscard]] std::string format(size_t loc_size, [[maybe_unused]] size_t longest_msg_size) const override {
        std::stringstream stream;
        stream << err_loc_msg;

        const size_t pad_size = (stream.str().size() < loc_size) ? 
                                    (loc_size - stream.str().size()) : 0;
        stream << ":" << std::string(pad_size, ' ') << RED << str << RESET;

        return stream.str();
    }

    [[nodiscard]] bool is_error_msg() const override { return true; }

    std::string err_loc_msg;
};

class parser_error_msg : virtual public error_msg {
    public:
    explicit parser_error_msg(std::string msg, token set_tok, const std::source_location& loc) : tok(set_tok) {
        std::stringstream stream;
        stream << GET_ERROR_LOCATION(loc);
        err_loc_msg = stream.str();

        str = msg;
    }
    
    [[nodiscard]] std::string format(size_t loc_size, size_t longest_msg_size) const override {
        
        std::stringstream stream;
        stream << err_loc_msg;

        const std::string& line_info = " .Line = "     + std::to_string(tok.line) +        
                                       ", position = " + std::to_string(tok.position);      

        const size_t file_loc_pad_size = (stream.str().size() < loc_size) ? 
                                            (loc_size - stream.str().size()) : 0;

        const size_t first_section_size = err_loc_msg.size() + file_loc_pad_size + str.size();

        const size_t query_loc_pad_size = (first_section_size < longest_msg_size) ? 
                                            (longest_msg_size - first_section_size) : 0;

        stream << ":" << std::string(file_loc_pad_size, ' ') << RED << str << std::string(query_loc_pad_size, ' ') << line_info << RESET;

        return stream.str();
    }

    token tok;
};

template<typename DefaultMsgType>
    requires std::derived_from<DefaultMsgType, message>
class logger {

    public:

    // static std::vector<message> get_raw_msgs() {
    //     return msgs;
    // }

    size_t msg_count() {
        return msgs.size(); }

    void clear_log() {
        msgs.clear(); }

    bool has_msgs() {
        return !msgs.empty(); }


    // Variadic template for flexible message creation
    template<typename... Args>
    void add_msg(Args&&... args) {
        msgs.push_back(std::make_unique<DefaultMsgType>(std::forward<Args>(args)...));
    }
    
    // Add pre-constructed message
    // void add_msg(std::unique_ptr<message> msg) {
    //     msgs.push_back(std::move(msg));
    // }
    
    // Add raw pointer (takes ownership)
    void add_msg(message* msg) {
        msgs.push_back(std::unique_ptr<message>(msg));
    }
    
    // Convenience method for different message types
    template<typename MsgType, typename... Args>
        requires std::derived_from<MsgType, message>
    void add_custom_msg(Args&&... args) {
        msgs.push_back(std::make_unique<MsgType>(std::forward<Args>(args)...));
    }

    template<typename MsgType, typename... Args>
        requires std::derived_from<MsgType, message>
    void add_msg(Args&&... args) {
        msgs.push_back(std::make_unique<MsgType>(std::forward<Args>(args)...));
    }


    std::string get_formated_msgs() {
        std::stringstream stream;

        size_t longest_loc_size = 0;
        size_t longest_msg_size = 0;
        for (const auto& msg : msgs) {
            size_t msg_size = msg->str.size();
            if (msg->is_error_msg()) {
                const error_msg* err_msg = dynamic_cast<const error_msg*>(msg.get());
                const size_t loc_size = err_msg->err_loc_msg.size();
                if (loc_size > longest_loc_size) {
                    longest_loc_size = loc_size; }

                msg_size += loc_size;
            }

            if (msg_size > longest_msg_size) {
                longest_msg_size = msg_size;
            }
        }

        for (const auto& msg : msgs) {
            stream << msg->format(longest_loc_size + 1, longest_msg_size + 1) << "\n"; } // Plus one for space

        return stream.str();
    }

    std::vector<std::string> get_formated_msgs_as_vec() {

        size_t last_space       = 0;
        size_t longest_msg_size = 0;
        for (const auto& msg : msgs) {
            if (msg->str.size() > longest_msg_size) {
                longest_msg_size = msg->str.size(); }

            for (size_t i = 0; i < msg->str.size(); i++) {
                const auto& charac = msg->str[i];
                if (std::isspace(charac)) {
                    if (i > last_space) {
                        last_space = i; }
                    break;
                }
            }
        }

        std::vector<std::string> ret;
        ret.reserve(msgs.size());
        for (const auto& msg : msgs) {
            ret.emplace_back(msg->format(last_space, longest_msg_size) + "\n"); }

        return ret;
    }

    private:
    std::vector<std::unique_ptr<message>> msgs;
};