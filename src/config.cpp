#include "config.h"

#include <Windows.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>

namespace {

std::atomic<std::shared_ptr<const ModConfig>> g_config{std::make_shared<const ModConfig>(ModConfig{})};
std::mutex g_config_path_mutex;
std::wstring g_config_path;

// ── INI reading primitives ─────────────────────────────────────────────

bool ReadBool(const wchar_t* section, const wchar_t* key, const bool default_value, const std::wstring& path) {
    return GetPrivateProfileIntW(section, key, default_value ? 1 : 0, path.c_str()) != 0;
}

bool HasIniKey(const wchar_t* section, const wchar_t* key, const std::wstring& path) {
    wchar_t buffer[2]{};
    return GetPrivateProfileStringW(
               section,
               key,
               L"",
               buffer,
               static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])),
               path.c_str()) > 0;
}

DWORD ReadDword(const wchar_t* section, const wchar_t* key, const DWORD default_value, const std::wstring& path) {
    return static_cast<DWORD>(GetPrivateProfileIntW(section, key, static_cast<int>(default_value), path.c_str()));
}

void ReadBoolIfPresent(const wchar_t* section,
                       const wchar_t* key,
                       bool* const target,
                       const std::wstring& path) {
    if (target != nullptr && HasIniKey(section, key, path)) {
        *target = ReadBool(section, key, *target, path);
    }
}

void ReadBoolAliasIfPresent(const wchar_t* section,
                            const wchar_t* canonical_key,
                            const wchar_t* alias_key,
                            bool* const target,
                            const std::wstring& path) {
    if (target != nullptr && (HasIniKey(section, canonical_key, path) ||
                             (alias_key != nullptr && HasIniKey(section, alias_key, path)))) {
        *target = ReadBool(section, canonical_key, *target, path);
    }
}

void ReadDwordIfPresent(const wchar_t* section,
                        const wchar_t* key,
                        DWORD* const target,
                        const std::wstring& path) {
    if (target != nullptr && HasIniKey(section, key, path)) {
        *target = ReadDword(section, key, *target, path);
    }
}

// ── Config layer application ───────────────────────────────────────────

void ApplyConfigLayer(const std::wstring& path, ModConfig* const next) {
    if (next == nullptr) {
        return;
    }

    // [General]
    ReadBoolIfPresent(L"General", L"Enabled", &next->general.enabled, path);
    ReadBoolIfPresent(L"General", L"LogEnabled", &next->general.log_enabled, path);
    ReadBoolIfPresent(L"General", L"Verbose", &next->general.verbose, path);
    ReadDwordIfPresent(L"General", L"MaxLogLines", &next->general.max_log_lines, path);
    ReadDwordIfPresent(L"General", L"InitDelayMs", &next->general.init_delay_ms, path);

    // [Shield]
    ReadBoolAliasIfPresent(L"Shield", L"Enabled", L"Enable", &next->shield.enabled, path);
}

void SanitizeConfig(ModConfig* const next) {
    if (next == nullptr) {
        return;
    }

    if (next->general.max_log_lines == 0) {
        next->general.max_log_lines = 2000;
    } else if (next->general.max_log_lines > 1000000) {
        next->general.max_log_lines = 1000000;
    }
}

}  // namespace

// ── Public API ─────────────────────────────────────────────────────────

bool ReadConfigSnapshot(const std::wstring& config_path, ModConfig* const config) {
    if (config == nullptr) {
        return false;
    }

    ModConfig next{};

    // Read single INI file directly (no layer merging for simplicity;
    // layers can be added later if needed)
    if (std::filesystem::is_regular_file(config_path)) {
        ApplyConfigLayer(config_path, &next);
    }

    SanitizeConfig(&next);
    *config = next;
    return true;
}

void SetConfigSnapshot(const std::wstring& config_path, const ModConfig& config) {
    g_config.store(std::make_shared<const ModConfig>(config), std::memory_order_release);

    std::lock_guard lock(g_config_path_mutex);
    g_config_path = config_path;
}

bool LoadConfig(const std::wstring& config_path) {
    ModConfig next{};
    if (!ReadConfigSnapshot(config_path, &next)) {
        return false;
    }

    SetConfigSnapshot(config_path, next);
    return true;
}

ModConfig GetConfig() {
    const auto snapshot = g_config.load(std::memory_order_acquire);
    if (!snapshot) {
        return {};
    }

    return *snapshot;
}

std::wstring GetLoadedConfigPath() {
    std::lock_guard lock(g_config_path_mutex);
    return g_config_path;
}
