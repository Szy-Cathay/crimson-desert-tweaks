#include "config_watcher.h"

#include "config.h"
#include "logger.h"

#include <Windows.h>

#include <atomic>

namespace {

constexpr DWORD kConfigWatchPollMs = 1000;
constexpr DWORD kConfigReloadSettleMs = 200;

std::atomic<bool> g_config_watcher_running{false};
HANDLE g_config_watcher_thread = nullptr;

bool TryGetLastWriteTimestamp(const std::wstring& path, ULONGLONG* const timestamp) {
    if (timestamp == nullptr) {
        return false;
    }

    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attributes)) {
        return false;
    }

    ULARGE_INTEGER last_write{};
    last_write.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
    last_write.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
    *timestamp = last_write.QuadPart;
    return true;
}

void ApplyLoggerReload(const ModConfig& previous, const ModConfig& current) {
    if (previous.general.log_enabled && !current.general.log_enabled) {
        Log("config-watcher: config reloaded, disabling logger");
        UpdateLoggerConfig(false, current.general.verbose, current.general.max_log_lines);
        return;
    }

    UpdateLoggerConfig(current.general.log_enabled, current.general.verbose, current.general.max_log_lines);
    if (current.general.log_enabled) {
        Log("config-watcher: config reloaded (verbose=%d max-log-lines=%lu shield=%d)",
            current.general.verbose ? 1 : 0,
            static_cast<unsigned long>(current.general.max_log_lines),
            current.shield.enabled ? 1 : 0);
    }
}

void ConfigWatcherLoop() {
    const std::wstring config_path = GetLoadedConfigPath();
    ULONGLONG last_write_timestamp = 0;
    TryGetLastWriteTimestamp(config_path, &last_write_timestamp);

    while (g_config_watcher_running.load(std::memory_order_acquire)) {
        Sleep(kConfigWatchPollMs);
        if (!g_config_watcher_running.load(std::memory_order_acquire)) {
            break;
        }

        ULONGLONG observed_timestamp = 0;
        if (!TryGetLastWriteTimestamp(config_path, &observed_timestamp) ||
            observed_timestamp == last_write_timestamp) {
            continue;
        }

        Sleep(kConfigReloadSettleMs);

        ULONGLONG settled_timestamp = 0;
        if (!TryGetLastWriteTimestamp(config_path, &settled_timestamp)) {
            continue;
        }

        last_write_timestamp = settled_timestamp;

        const ModConfig previous = GetConfig();
        ModConfig next{};
        if (!ReadConfigSnapshot(config_path, &next)) {
            if (previous.general.log_enabled) {
                Log("config-watcher: failed to read updated config");
            }
            continue;
        }

        if (next == previous) {
            continue;
        }

        SetConfigSnapshot(config_path, next);
        ApplyLoggerReload(previous, next);

        // Hook loadout changes require restart
        if (ShouldInstallShieldHideHook(previous) != ShouldInstallShieldHideHook(next) &&
            next.general.log_enabled) {
            Log("config-watcher: hook loadout changed; restart game to apply");
        }
    }
}

DWORD WINAPI ConfigWatcherThreadProc(LPVOID) {
    ConfigWatcherLoop();
    return 0;
}

}  // namespace

bool StartConfigWatcher() {
    if (g_config_watcher_running.exchange(true, std::memory_order_acq_rel)) {
        return true;
    }

    const std::wstring config_path = GetLoadedConfigPath();
    if (config_path.empty()) {
        g_config_watcher_running.store(false, std::memory_order_release);
        return false;
    }

    g_config_watcher_thread = CreateThread(nullptr, 0, ConfigWatcherThreadProc, nullptr, 0, nullptr);
    if (g_config_watcher_thread == nullptr) {
        g_config_watcher_running.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void StopConfigWatcher() {
    if (!g_config_watcher_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    if (g_config_watcher_thread != nullptr) {
        WaitForSingleObject(g_config_watcher_thread, INFINITE);
        CloseHandle(g_config_watcher_thread);
        g_config_watcher_thread = nullptr;
    }
}
