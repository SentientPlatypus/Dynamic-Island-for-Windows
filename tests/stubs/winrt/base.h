// Minimal stand-in so the mod can be type-checked with mingw, which does not
// ship C++/WinRT. The real mod is compiled by Windhawk against the Windows SDK.
#pragma once
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>

namespace winrt {
template <typename T>
struct async_t {
    T get() const { return T{}; }
};
template <>
struct async_t<void> {
    void get() const {}
};
template <>
struct async_t<bool> {
    bool get() const { return false; }
};

struct hstring {
    hstring() = default;
    hstring(const wchar_t*) {}
    const wchar_t* c_str() const { return L""; }
};

enum class apartment_type { single_threaded, multi_threaded };
inline void init_apartment(apartment_type = apartment_type::multi_threaded) {}
inline void uninit_apartment() {}

struct clock {
    struct ticks {
        int64_t count() const { return 0; }
        ticks operator-(ticks const&) const { return {}; }
    };
    static ticks now() { return {}; }
};

template <typename T>
struct array_view {
    array_view(T*, T*) {}
};

struct hresult_error : std::exception {
    const wchar_t* message() const { return L""; }
    int32_t to_abi() const { return 0; }
};
}  // namespace winrt
