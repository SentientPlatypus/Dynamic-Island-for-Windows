#pragma once
#include "Windows.Storage.Streams.h"
namespace winrt::Windows::Media::Control {
enum class GlobalSystemMediaTransportControlsSessionPlaybackStatus {
    Closed, Opened, Changing, Stopped, Playing, Paused
};
struct TimeSpan {
    int64_t count() const { return 0; }
};
struct GlobalSystemMediaTransportControlsSessionTimelineProperties {
    TimeSpan Position() const { return {}; }
    TimeSpan EndTime() const { return {}; }
    winrt::clock::ticks LastUpdatedTime() const { return {}; }
    explicit operator bool() const { return true; }
};
struct GlobalSystemMediaTransportControlsSessionPlaybackInfo {
    GlobalSystemMediaTransportControlsSessionPlaybackStatus PlaybackStatus() const {
        return GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed;
    }
};
struct GlobalSystemMediaTransportControlsSessionMediaProperties {
    winrt::hstring Title() const { return {}; }
    winrt::hstring Artist() const { return {}; }
    winrt::hstring AlbumTitle() const { return {}; }
    winrt::Windows::Storage::Streams::IRandomAccessStreamReference Thumbnail() const { return {}; }
};
struct GlobalSystemMediaTransportControlsSession {
    GlobalSystemMediaTransportControlsSession() = default;
    GlobalSystemMediaTransportControlsSession(std::nullptr_t) {}
    GlobalSystemMediaTransportControlsSession& operator=(std::nullptr_t) { return *this; }
    explicit operator bool() const { return false; }
    GlobalSystemMediaTransportControlsSessionPlaybackInfo GetPlaybackInfo() const { return {}; }
    winrt::hstring SourceAppUserModelId() const { return {}; }
    winrt::async_t<GlobalSystemMediaTransportControlsSessionMediaProperties> TryGetMediaPropertiesAsync() const { return {}; }
    GlobalSystemMediaTransportControlsSessionTimelineProperties GetTimelineProperties() const { return {}; }
    winrt::async_t<bool> TryChangePlaybackPositionAsync(int64_t) const { return {}; }
    winrt::async_t<bool> TrySkipPreviousAsync() const { return {}; }
    winrt::async_t<bool> TryTogglePlayPauseAsync() const { return {}; }
    winrt::async_t<bool> TrySkipNextAsync() const { return {}; }
};
struct SessionList {
    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = GlobalSystemMediaTransportControlsSession;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type;
        bool operator==(iterator) const { return true; }
        bool operator!=(iterator) const { return false; }
        iterator& operator++() { return *this; }
        value_type operator*() const { return {}; }
    };
    iterator begin() const { return {}; }
    iterator end() const { return {}; }
};
struct GlobalSystemMediaTransportControlsSessionManager {
    GlobalSystemMediaTransportControlsSessionManager() = default;
    GlobalSystemMediaTransportControlsSessionManager(std::nullptr_t) {}
    GlobalSystemMediaTransportControlsSessionManager& operator=(std::nullptr_t) { return *this; }
    explicit operator bool() const { return false; }
    static winrt::async_t<GlobalSystemMediaTransportControlsSessionManager> RequestAsync() { return {}; }
    GlobalSystemMediaTransportControlsSession GetCurrentSession() const { return {}; }
    SessionList GetSessions() const { return {}; }
};
}
