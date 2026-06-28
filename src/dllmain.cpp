#include "config.h"
#include "config_watcher.h"
#include "logger.h"

#include <safetyhook.hpp>

#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <string>

// ── Forward declarations ──────────────────────────────────────────────

bool InstallShieldHideHook();
void RemoveShieldHideHook();

// ── Anonymous namespace ───────────────────────────────────────────────

namespace {

HMODULE g_module = nullptr;
constexpr wchar_t kTargetProcessName[] = L"CrimsonDesert.exe";
bool g_started_in_target_process = false;

#ifndef CRIMSON_DESERT_TWEAKS_BUILD_ID
#define CRIMSON_DESERT_TWEAKS_BUILD_ID "dev"
#endif

constexpr char kModSourceBuildId[] = CRIMSON_DESERT_TWEAKS_BUILD_ID;

std::atomic<DWORD> g_last_init_exception_code{0};
std::atomic<std::uintptr_t> g_last_init_exception_address{0};

// ── Path helpers ──────────────────────────────────────────────────────

std::wstring GetSiblingPath(const wchar_t* file_name) {
    std::wstring module_path(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(g_module, module_path.data(), static_cast<DWORD>(module_path.size()));
    module_path.resize(length);

    auto path = std::filesystem::path(module_path);
    path.replace_filename(file_name);
    return path.wstring();
}

std::wstring GetHostProcessPath() {
    std::wstring process_path(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, process_path.data(), static_cast<DWORD>(process_path.size()));
    process_path.resize(length);
    return process_path;
}

bool IsTargetHostProcess(const std::wstring& process_path) {
    if (process_path.empty()) {
        return false;
    }

    const auto file_name = std::filesystem::path(process_path).filename().wstring();
    return _wcsicmp(file_name.c_str(), kTargetProcessName) == 0;
}

// ── Config logging ────────────────────────────────────────────────────

void LogEffectiveConfig(const ModConfig& config) {
    Log("config: General Enabled=%d LogEnabled=%d Verbose=%d MaxLogLines=%lu InitDelayMs=%lu",
        config.general.enabled ? 1 : 0,
        config.general.log_enabled ? 1 : 0,
        config.general.verbose ? 1 : 0,
        static_cast<unsigned long>(config.general.max_log_lines),
        static_cast<unsigned long>(config.general.init_delay_ms));
    Log("config: Shield Enabled=%d",
        config.shield.enabled ? 1 : 0);
}

// ── Init / cleanup ────────────────────────────────────────────────────

void CleanupAfterInitializationFailure() {
    StopConfigWatcher();
    RemoveShieldHideHook();
}

int CaptureInitializeException(EXCEPTION_POINTERS* exception_info) {
    DWORD code = 0;
    std::uintptr_t address = 0;
    if (exception_info != nullptr && exception_info->ExceptionRecord != nullptr) {
        code = exception_info->ExceptionRecord->ExceptionCode;
        address = reinterpret_cast<std::uintptr_t>(exception_info->ExceptionRecord->ExceptionAddress);
    }
    g_last_init_exception_code.store(code, std::memory_order_relaxed);
    g_last_init_exception_address.store(address, std::memory_order_relaxed);
    return EXCEPTION_EXECUTE_HANDLER;
}

DWORD InitializeModUnchecked() {
    const auto host_process_path = GetHostProcessPath();
    if (!IsTargetHostProcess(host_process_path)) {
        return 0;
    }

    const auto config_path = GetSiblingPath(L"crimson-desert-tweaks.ini");
    const auto log_path = GetSiblingPath(L"crimson-desert-tweaks.log");

    LoadConfig(config_path);
    const ModConfig initial_config = GetConfig();
    InitializeLogger(log_path,
                     initial_config.general.log_enabled,
                     initial_config.general.verbose,
                     initial_config.general.max_log_lines);

    Log("dllmain: initialization started");
    Log("dllmain: module base = 0x%p", reinterpret_cast<void*>(GetModuleHandleW(nullptr)));
    Log("dllmain: source build = %s compiled=%s %s",
        kModSourceBuildId,
        __DATE__,
        __TIME__);
    Log("dllmain: host process = %ls", host_process_path.c_str());
    Log("dllmain: config path = %ls", config_path.c_str());
    LogEffectiveConfig(initial_config);

    const auto init_delay = initial_config.general.init_delay_ms;
    if (init_delay > 0) {
        Sleep(init_delay);
    }

    // Install hooks
    if (!InstallShieldHideHook()) {
        Log("dllmain: shield-hide hook unavailable; continuing");
    }

    // Start config watcher for hot-reload
    if (!StartConfigWatcher()) {
        Log("dllmain: config watcher failed to start");
    }

    Log("dllmain: initialization finished");
    return 0;
}

DWORD InitializeModBody() {
    try {
        return InitializeModUnchecked();
    } catch (...) {
        Log("dllmain: C++ exception during initialization; disabling mod for this process");
        CleanupAfterInitializationFailure();
        return 0;
    }
}

DWORD WINAPI InitializeMod(LPVOID) {
    __try {
        return InitializeModBody();
    } __except (CaptureInitializeException(GetExceptionInformation())) {
        const auto code = g_last_init_exception_code.load(std::memory_order_relaxed);
        const auto address = g_last_init_exception_address.load(std::memory_order_relaxed);
        Log("dllmain: SEH exception during initialization code=0x%08lX address=0x%p; disabling mod for this process",
            static_cast<unsigned long>(code),
            reinterpret_cast<void*>(address));
        CleanupAfterInitializationFailure();
        return 0;
    }
}

}  // namespace

// ── DLL entry point ───────────────────────────────────────────────────

BOOL APIENTRY DllMain(HMODULE module, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);

        if (!IsTargetHostProcess(GetHostProcessPath())) {
            return TRUE;
        }

        g_started_in_target_process = true;
        HANDLE thread = CreateThread(nullptr, 0, InitializeMod, nullptr, 0, nullptr);
        if (thread != nullptr) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        if (!g_started_in_target_process) {
            return TRUE;
        }

        Log("dllmain: process detach started");
        StopConfigWatcher();
        RemoveShieldHideHook();
        Log("dllmain: process detach finished");
        ShutdownLogger();
    }

    return TRUE;
}
