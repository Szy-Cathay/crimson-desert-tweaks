[English](README.md) | [简体中文](README.zh-CN.md)

# Crimson Desert Tweaks

[![Language](https://img.shields.io/badge/language-C%2B%2B23-%23f34b7d)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-0078d7)](https://www.microsoft.com/windows)
[![Game](https://img.shields.io/badge/game-Crimson%20Desert-orange)](https://crimsondesert.pearlabyss.com/)

A collection of quality-of-life tweaks for **Crimson Desert**, implemented as an ASI plugin via runtime memory hooks.

> More features coming soon — this project is under active exploration.

---

## Features

| Feature | Description |
|---------|-------------|
| **Shield Auto-Hide** | Automatically hide back shield when dual-wielding |

## Installation

1. Make sure you have an ASI loader installed (e.g. `version.dll` or `winmm.dll`)
2. Place `crimson-desert-tweaks.asi` and `crimson-desert-tweaks.ini` in the game's `bin64` directory

## Configuration

Edit `crimson-desert-tweaks.ini`. Changes take effect automatically (hot reload).

```ini
[General]
Enabled=1                   # Master enable
LogEnabled=1                # Logging
Verbose=0                   # Verbose log (set 1 for debugging)
MaxLogLines=2000
InitDelayMs=3000

[Shield]
Enable=1                    # Shield auto-hide
```

## Build

**Requirements**: Visual Studio 2022+, CMake 3.20+

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Tech Stack

- C++23 / MSVC x64
- [SafetyHook](https://github.com/cursey/safetyhook) — Hooking framework
- [Zydis](https://github.com/zyantific/zydis) — Disassembler

## Credits

- Developed using Claude Code (Anthropic)
- Built with [SafetyHook](https://github.com/cursey/safetyhook) and [Zydis](https://github.com/zyantific/zydis)
