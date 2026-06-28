#include "config.h"
#include "logger.h"
#include "scanner.h"

#include <safetyhook.hpp>

#include <Windows.h>

#include <atomic>
#include <cstdint>

namespace {

SafetyHookMid g_shield_fake_equip_hook{};

constexpr uintptr_t kMinimumPointerAddress = 0x10000;

// ── Hook callback ────────────────────────────────────────────────────────

// Hook at "mov rcx,[rax]" that loads the FakeEquipTypeEntry pointer from the
// ShieldFakeEquip key global.
//
// FakeEquipTypeEntry (0x30 bytes):
//   +0x00: name_ptr   (self-ref → +0x18)
//   +0x08: slot_type  (int32, 15 = ShieldFakeEquip)
//   +0x0C: unk_ff     (int32, -1)
//   +0x10: flags      (int64, 1 = enabled)
//   +0x18: name_buf[] ("ShieldFakeEquip\0")

void ShieldFakeEquipCallback(SafetyHookContext& ctx) {
    const uintptr_t base = ctx.rax;
    if (base < kMinimumPointerAddress) {
        return;
    }

    const int32_t slot_type = *reinterpret_cast<const int32_t*>(base + 0x08);
    if (slot_type != 15) {
        return;
    }

    if (!GetConfig().shield.enabled) {
        return;
    }

    // Zero the flags field so that every code path sees this entry as disabled,
    // then redirect rax to a dummy so the current call also gets a null entry.
    *reinterpret_cast<int64_t*>(base + 0x10) = 0;

    static const uintptr_t kDummyZero = 0;
    ctx.rax = reinterpret_cast<uintptr_t>(&kDummyZero);
}

}  // namespace

// ── Public install / remove ──────────────────────────────────────────────

bool InstallShieldHideHook() {
    if (!GetConfig().shield.enabled) {
        return true;
    }

    const uintptr_t target = ScanForShieldFakeEquipAccess();
    if (target == 0) {
        Log("shield: failed to find ShieldFakeEquip access point");
        return false;
    }

    auto hook_result = SafetyHookMid::create(reinterpret_cast<void*>(target), ShieldFakeEquipCallback);
    if (!hook_result.has_value()) {
        Log("shield: failed to create mid hook");
        return false;
    }

    g_shield_fake_equip_hook = std::move(*hook_result);
    Log("shield: installed hook at 0x%p", reinterpret_cast<void*>(target));
    return true;
}

void RemoveShieldHideHook() {
    if (g_shield_fake_equip_hook) {
        g_shield_fake_equip_hook.reset();
        Log("shield: removed hook");
    }
}
