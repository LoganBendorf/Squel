#pragma once



template <std::integral Numeric>
static Numeric raw_hash(const char* blob, size_t size, bool debug) {
    
    if (debug) [[unlikely]] { std::cout << "raw_hash() received: " << blob << std::endl; }
    
    Numeric hash = 5381;
    for (size_t i = 0; i < size; i++) {

        char cur_char = static_cast<char>(std::tolower(blob[i]));

        // printf("hash = %d\n", hash);

        // If emoji
        if (cur_char >> 7 == 1) {
            if (debug) [[unlikely]] { std::cout << "raw_hash() received an emoji" << std::endl; }
            hash = ((hash << 5) + hash) + 'o';
            i += 3;
            continue;
        }

        bool shouldSkip = false;
        switch (cur_char) {
            case ' ': shouldSkip = true; break;
            case '_': shouldSkip = true; break;
            case '(': if (i + 1 < size && cur_char == ')') {
                hash = ((hash << 5) + hash) + 'o';
                i++;
                shouldSkip = true;
            } break;
            case '0': cur_char = 'o'; break;
            case '@': cur_char = 'a'; break;
            case 'i': cur_char = 'l'; break;
            case '|': cur_char = 'l'; break;
            case '!': cur_char = 'l'; break;
            case '3': cur_char = 'e'; break;
            case '$': cur_char = 's'; break;
            default: break;
        }

        if (shouldSkip) { continue; }

        hash = ((hash << 5) + hash) + cur_char;
    }

    if (hash == 0) {
        if (debug) [[unlikely]] { std::cout << "hash equaled 0, incrementing it by 1" << std::endl; }
        return 1;
    }

    return hash;
}