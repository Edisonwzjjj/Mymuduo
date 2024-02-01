//
// Created by ZIJUN WANG on 1/2/2024.
//

#include "Any.h"

template<typename T>
Any::Any(const T &val): content_(new Holder<T>(val)) {}

Any::Any(const Any &other) : content_(other.content_ ? other.content_->Clone() : nullptr) {}

Any::~Any() {
    delete content_;
}

auto Any::Type() -> const std::type_info & {
    return content_ ? content_->Type() : typeid(void);
}

template<typename T>
auto Any::Get() -> T * {
    assert(content_->Type() == typeid(T));
    return dynamic_cast<Holder<T> *>(content_)->val_;
}


auto Any::Swap(Any &other) -> Any & {
    std::swap(content_, other.content_);
    return *this;
}

template<typename T>
Any &Any::operator=(const T &val) {
    Any(val).Swap(*this);
    return *this;
}


Any &Any::operator=(Any other) {
    Any(other).Swap(other);
    return *this;
}








