#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

// ── Config structures ──────────────────────────────────────────────────

struct ShieldConfig {
    bool enabled = false;

    bool operator==(const ShieldConfig&) const = default;
};

struct GeneralConfig {
    bool enabled = true;
    bool log_enabled = true;
    bool verbose = false;
    DWORD max_log_lines = 2000;
    DWORD init_delay_ms = 3000;

    bool operator==(const GeneralConfig&) const = default;
};

struct ModConfig {
    GeneralConfig general;
    ShieldConfig shield;

    bool operator==(const ModConfig&) const = default;
};

// ── Helpers ────────────────────────────────────────────────────────────

inline bool ShouldInstallShieldHideHook(const ModConfig& config) {
    return config.general.enabled && config.shield.enabled;
}

// ── Config I/O ─────────────────────────────────────────────────────────

bool LoadConfig(const std::wstring& config_path);
bool ReadConfigSnapshot(const std::wstring& config_path, ModConfig* config);
void SetConfigSnapshot(const std::wstring& config_path, const ModConfig& config);
ModConfig GetConfig();
std::wstring GetLoadedConfigPath();
