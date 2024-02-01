#pragma once

#ifndef MYMUDUO_ANY_H
#define MYMUDUO_ANY_H

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
        virtual ~PlaceHolder() = 0;

        virtual const std::type_info &Type() = 0;

        virtual PlaceHolder *Clone() = 0;
    };

    template<typename T>
    class Holder : public PlaceHolder {
    public:
        T val_;
    public:
        explicit Holder(const T &val) : val_(val) {};

        ~Holder() override = default;

        const std::type_info &Type() override { return typeid(T); }

        PlaceHolder *Clone() override { return new Holder(val_); }
    };

    PlaceHolder *content_{nullptr};
public:
    Any() = default;

    template<typename T>
    explicit Any(const T &val);

    Any(const Any &other);

    ~Any();

    auto Type() -> const std::type_info &;

    template<typename T>
    auto Get() -> T *;

    template<typename T>
    Any &operator=(const T &val);

    Any &operator=(Any other);

private:
    auto Swap(Any &other) -> Any &;

};


#endif //MYMUDUO_ANY_H
