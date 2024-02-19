#pragma once
#include <utility>
#include <memory>
#include "MyNew.h"

template <typename T>
struct CustomDeleter {
    void operator()(T* ptr) const {
        Delete<T>(ptr);
    }
};
template<typename T>
using UniquePtr = std::unique_ptr<T, CustomDeleter<T>>;
template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args)
{
    UniquePtr <T> ptr(New<T>(forward<Args>(args)...));
    return ptr;
}