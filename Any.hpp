#pragma once

#ifndef MYMUDUO_ANY_HPP
#define MYMUDUO_ANY_HPP

#include <iostream>
#include <string>
#include <cassert>
#include <typeinfo>
#include <typeindex>
#include <unistd.h>

class Any {
private:
    class PlaceHolder {
    public:
        virtual ~PlaceHolder() = default;

        virtual const std::type_info &Type() = 0;

        virtual PlaceHolder *Clone() = 0;
    };

    template<typename T>
    class Holder : public PlaceHolder {
    public:
        T val_;
    public:
        explicit Holder(const T &val) : val_(val) {};

        const std::type_info &Type() override { return typeid(T); }

        PlaceHolder *Clone() override { return new Holder(val_); }
    };

    PlaceHolder *content_{nullptr};
public:
    Any() = default;

    template<typename T>
    explicit Any(const T &val): content_(new Holder<T>(val)) {}

    Any(const Any &other): content_(other.content_ ? other.content_->Clone() : nullptr) {}

    ~Any() {
        delete content_;
    }

    auto Type() -> const std::type_info & {
        return content_->Type();
    }

    template<typename T>
    auto Get() -> T * {
        assert(typeid(T) == content_->Type());
        return &((Holder<T> *)content_)->val_;
    }

    template<typename T>
    Any &operator=(const T &val) {
        Any(val).Swap(*this);
        return *this;
    }

    Any &operator=(const Any& other) {
        Any(other).Swap(*this);
        return *this;
    }

private:
    auto Swap(Any &other) -> Any & {
        std::swap(content_, other.content_);
        return *this;
    }

};


#endif //MYMUDUO_ANY_HPP
