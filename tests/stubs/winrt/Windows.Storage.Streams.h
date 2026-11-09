#pragma once
#include "base.h"
namespace winrt::Windows::Storage::Streams {
struct IInputStream {};
struct IRandomAccessStream {
    uint64_t Size() const { return 0; }
    IInputStream GetInputStreamAt(uint64_t) const { return {}; }
    explicit operator bool() const { return false; }
};
struct IRandomAccessStreamReference {
    winrt::async_t<IRandomAccessStream> OpenReadAsync() const { return {}; }
    explicit operator bool() const { return false; }
};
struct IBuffer {};
struct DataReader {
    DataReader() = default;
    explicit DataReader(IInputStream) {}
    winrt::async_t<uint32_t> LoadAsync(uint32_t) const { return {}; }
    void ReadBytes(winrt::array_view<uint8_t>) const {}
    static DataReader FromBuffer(IBuffer const&) { return {}; }
};
}
