[English](README.md) | [简体中文](README.zh-CN.md)

# Crimson Desert Tweaks / 红色沙漠功能微调

[![Language](https://img.shields.io/badge/language-C%2B%2B23-%23f34b7d)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-0078d7)](https://www.microsoft.com/windows)
[![Game](https://img.shields.io/badge/game-Crimson%20Desert-orange)](https://crimsondesert.pearlabyss.com/)

**Crimson Desert** 的 ASI 功能微调模组合集，通过运行时内存 Hook 实现各种游戏体验优化。

> 更多功能持续探索中，敬请期待。

---

## 功能列表

| 功能 | 说明 |
|------|------|
| **盾牌自动隐藏** | 双持武器时自动隐藏背后的盾牌 |

## 安装方法

1. 确保游戏已安装 ASI 加载器（如 `version.dll` 或 `winmm.dll`）
2. 将 `crimson-desert-tweaks.asi` 和 `crimson-desert-tweaks.ini` 放入游戏 `bin64` 目录

## 配置文件

编辑 `crimson-desert-tweaks.ini`，保存后自动生效（支持热重载）。

```ini
[General]
Enabled=1                   # 总开关
LogEnabled=1                # 日志开关
Verbose=0                   # 详细日志（调试时设为 1）
MaxLogLines=2000
InitDelayMs=3000

[Shield]
Enable=1                    # 盾牌自动隐藏
```

## 构建

**环境要求**：Visual Studio 2022+、CMake 3.20+

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```

## 技术栈

- C++23 / MSVC x64
- [SafetyHook](https://github.com/cursey/safetyhook) — Hook 框架
- [Zydis](https://github.com/zyantific/zydis) — 反汇编引擎

## 致谢

- 使用 Claude Code (Anthropic) 开发
- 基于 [SafetyHook](https://github.com/cursey/safetyhook) 和 [Zydis](https://github.com/zyantific/zydis) 构建
