#ifndef VTYPES_HPP
#define VTYPES_HPP
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

namespace vtypes {

class VString : public std::string {
public:
    using std::string::string;
    VString() : std::string() {}
    VString(const std::string &s) : std::string(s) {}
    VString(const char* s) : std::string(s) {}

    VString operator+(const VString &other) const {
        return VString(static_cast<const std::string&>(*this) + static_cast<const std::string&>(other));
    }

    VString operator+(const std::string &other) const {
        return VString(static_cast<const std::string&>(*this) + other);
    }

    friend VString operator+(const std::string &lhs, const VString &rhs) {
        return VString(lhs + static_cast<const std::string&>(rhs));
    }

    VString operator*(long long times) const {
        if (times < 0) throw std::invalid_argument("Multiplier must be non-negative");
        VString result;
        result.reserve(this->size() * (size_t)times);
        for (long long i = 0; i < times; ++i) result += *this;
        return result;
    }

    friend VString operator*(long long times, const VString &s) {
        return s * times;
    }

    VString& operator+=(const VString &other) {
        static_cast<std::string&>(*this) += static_cast<const std::string&>(other);
        return *this;
    }

    VString& operator+=(const std::string &other) {
        static_cast<std::string&>(*this) += other;
        return *this;
    }

    VString& operator*=(long long times) {
        if (times < 0) throw std::invalid_argument("Multiplier must be non-negative");
        VString tmp = *this;
        this->clear();
        for (long long i = 0; i < times; ++i) *this += tmp;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream &os, const VString &s) {
        os << static_cast<const std::string&>(s);
        return os;
    }
};

class VList {
public:
    std::vector<VString> items;
    VList() : items(), _scalar_from_string(false) {}
    VList(std::initializer_list<VString> init) : items(init), _scalar_from_string(false) {}
    size_t size() const { return items.size(); }
    VString operator[](size_t i) const { return items.at(i); }
    VString& operator[](size_t i) { return items.at(i); }
    void add_inplace(size_t idx, const VString& val) {
        if (idx > items.size()) idx = items.size();
        items.insert(items.begin() + idx, val);
        _scalar_from_string = false;
    }
    VString remove(size_t idx) {
        VString v = items.at(idx);
        items.erase(items.begin() + idx);
        _scalar_from_string = false;
        return v;
    }
    VList& remove_inplace(size_t idx) {
        items.erase(items.begin() + idx);
        _scalar_from_string = false;
        return *this;
    }
    void push_inplace(const VString& val) {
        items.push_back(val);
        _scalar_from_string = false;
    }
    VString pop() {
        VString v = items.back();
        items.pop_back();
        _scalar_from_string = false;
        return v;
    }
    VList& pop_inplace() {
        if (!items.empty()) {
            items.pop_back();
        }
        _scalar_from_string = false;
        return *this;
    }
    void replace_inplace(size_t idx, const VString& val) {
        items.at(idx) = val;
        _scalar_from_string = false;
    }
    VList& operator=(const VString& s) {
        items.clear();
        items.push_back(s);
        _scalar_from_string = true;
        return *this;
    }
    VList& operator=(VString&& s) {
        items.clear();
        items.push_back(std::move(s));
        _scalar_from_string = true;
        return *this;
    }
    friend std::ostream& operator<<(std::ostream &os, const VList &l) {
        if (l._scalar_from_string && l.items.size() == 1) {
            os << l.items[0];
        } else {
            os << "[";
            for (size_t i = 0; i < l.items.size(); ++i) { if (i) os << ", "; os << l.items[i]; }
            os << "]";
        }
        return os;
    }
private:
    bool _scalar_from_string;
};

} // namespace vtypes

#endif
