#ifndef VCORE_HPP
#define VCORE_HPP

#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include "vtypes.hpp"

namespace vcore {
    template<typename T>
    inline void print(const T& value) {
        std::cout << value << '\n';
    }

    template<>
    inline void print<bool>(const bool& value) {
        std::cout << (value ? "true" : "false") << '\n';
    }

    template<>
    inline void print<char*>(char* const& value) {
        if (value == nullptr) {
            std::cout << "nullptr\n";
        } else {
            std::cout << value << '\n';
        }
    }

    template<>
    inline void print<const char*>(const char* const& value) {
        if (value == nullptr) {
            std::cout << "nullptr\n";
        } else {
            std::cout << value << '\n';
        }
    }

    template<>
    inline void print<std::string>(const std::string& value) {
        std::cout << value << '\n';
    }
    template<typename T, typename... Args>
    inline void print(const T& first, const Args&... args) {
        std::cout << first;
        ((std::cout << ' ' << args), ...);
        std::cout << '\n';
    }

    template<typename T>
    inline T add(const T& a, const T& b) {
        return a + b;
    }

    template<typename T>
    inline T sub(const T& a, const T& b) {
        return a - b;
    }

    template<typename T>
    inline T mul(const T& a, const T& b) {
        return a * b;
    }

    template<typename T>
    inline T div(const T& a, const T& b) {
        return a / b;
    }

    inline long long velox_to_int(const vtypes::VString &s) {
        const char* p = s.c_str();
        while (*p && std::isspace((unsigned char)*p)) ++p;
        const char* q = s.c_str() + s.size();
        while (q > p && std::isspace((unsigned char)*(q - 1))) --q;
        std::string trimmed(p, q);

        if (trimmed.empty()) return 0;

        char* endptr = nullptr;
        errno = 0;
        long long value = std::strtoll(trimmed.c_str(), &endptr, 10);
        if (endptr == trimmed.c_str()) {
            return 0;
        }
        if (errno == ERANGE) {
            if (trimmed.size() > 0 && trimmed[0] == '-') {
                return std::numeric_limits<long long>::min();
            } else {
                return std::numeric_limits<long long>::max();
            }
        }

        return value;
    }
}

#endif
