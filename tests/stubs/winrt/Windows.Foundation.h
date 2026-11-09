#pragma once
#include "base.h"
namespace winrt::Windows::Foundation {
struct IInspectable {
    explicit operator bool() const { return false; }
};
}
