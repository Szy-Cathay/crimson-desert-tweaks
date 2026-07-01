# Crimson Desert Tweaks — 项目规则

## 开工必读清单（先读这里）

在本项目中操作前，先遵守这几条硬规则：

1. **先说明再动手**：编码前先用中文写明假设、修改步骤、验证方式；不确定就先问。
2. **只改必要内容**：每一行改动都必须能对应用户当前需求，不顺手重构、不顺手清理无关代码。
3. **不自动安装产物**：构建输出只留在 `build/Release/`，不要复制到游戏目录或模组目录。
4. **源码改动要更新进度**：改 `src/` 下源码时，实时更新 `PROGRESS.md`，写清做了什么、发现了什么、下一步。
5. **配置别名不能破坏**：修改配置读取时，`Enable`/`Enabled` 这类别名必须继续走 `ReadBoolAlias` 逻辑。
6. **Hook 拦截优先**：遇到项目 Hook 拦截时，先停下说明原因，不要尝试绕过。
7. **Hook 误拦截处理**：如果怀疑 Hook 误拦截，除非用户已在当前对话明确要求修改该 Hook，否则必须先询问用户；得到明确同意后才允许改 Hook 或放宽规则。
8. **不更新 README 记流水账**：工作进度只写 `PROGRESS.md`；README 只在用户明确要求或公开说明确实变化时更新。
9. **项目规则随仓库维护**：`CLAUDE.md`、`.claude/`、`PROGRESS.md` 属于项目规则/进度文件，随仓库提交和维护。

## 项目概述

ASI 模组（DLL），通过 mid-function hook 修改 Crimson Desert 的内存数据。采用功能模块化设计，每个功能独立管理。

- **目标游戏**: Crimson Desert
- **语言**: C++23 / MSVC x64
- **Hook 框架**: safetyhook (含 Zydis 反汇编引擎)
- **构建**: CMake + Visual Studio 2022+

## 构建

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
# 输出目录: build/Release/
#   crimson-desert-tweaks.asi
# 自动复制到模组安装目录:
#   D:/红色沙漠工具/模组/Crimson Desert Tweaks/
#   ├── crimson-desert-tweaks.asi
#   └── crimson-desert-tweaks.ini  （项目根目录的 .ini 模板自动同步）
```

**注意**：项目根目录的 `crimson-desert-tweaks.ini` 是默认配置模板，构建时自动复制到输出目录。用户通过 DMM 安装后可在模组目录中修改配置。

首次构建需要从 GitHub 下载 Zydis。若网络不通，参考下方「联网操作规则」章节。

## 功能列表

| 功能 | 状态 | 说明 |
|------|------|------|
| Shield Hide | ✅ 已实现 | 双持时自动隐藏背后盾牌 |

## 架构

### Hook 层级

每个功能模块独立管理自己的 Hook，模块之间无依赖。

```
游戏函数 → [Hook 拦截] → 回调检查配置 → 修改内存 → 返回
```

### 模块结构

```
src/
├── dllmain.cpp          # 入口 + 初始化
├── config.cpp/h         # INI 配置读取 + 快照
├── config_watcher.cpp/h # 配置热重载（后台线程）
├── logger.cpp/h         # 日志输出
├── scanner.cpp/h        # AOB 签名扫描引擎
├── version.rc           # 版本资源
└── hooks/
    └── shield_hide.cpp  # 盾牌隐藏 Hook
```

### 配置

配置文件 `crimson-desert-tweaks.ini` 放在 ASI 同目录。

```ini
[General]
Enabled=1
LogEnabled=1
Verbose=0
MaxLogLines=2000
InitDelayMs=3000

[Shield]
Enable=1
```

## 修改规则

### 1. 添加新功能时

1. 在 `src/hooks/` 中创建新 Hook 回调文件
2. 在 `src/scanner.cpp` 中添加 AOB 签名扫描函数
3. 在 `src/scanner.h` 中声明扫描函数
4. 在 `src/config.h` 中添加配置结构 + helper
5. 在 `src/config.cpp` 的 `ApplyConfigLayer` 中添加段落读取
6. 在 `src/dllmain.cpp` 中注册安装/卸载 + 日志输出
7. 在项目根目录 `CMakeLists.txt` 中添加源文件
8. 更新 `PROGRESS.md`

### 2. AOB 签名规则

- 每个扫描目标提供签名：优先宽松匹配（含 `??`），精确匹配作后备
- 签名必须包含足够的上下文字节以确保唯一性（至少 12-16 字节）
- 新版本适配时：先在 `[General]` 中设置 `Verbose=1`，运行后查看日志中的 `DIAG` 行

### 3. Hook 安装规则

- 每个 Hook 模块提供 `Install*Hook()` 和 `Remove*Hook()` 函数
- 安装函数先检查配置，若功能未启用则直接返回 true
- 安装失败不致命，记录日志后继续

### 4. 配置键别名规则 ⚠️

`ReadBoolAliasIfPresent` 的别名参数（如 `Enable`/`Enabled`）需要配合 `ReadBoolAlias` 使用，**不能直接用 `ReadBool` 读 canonical key**。

错误写法：
```cpp
// 只检测了 alias 存在，但始终读 canonical key
// 如果 INI 写的是 Enable=1，HasIniKey("Enable")=true
// 但 ReadBool("Enabled") 因键不存在返回默认值 false → bug
*target = ReadBool(section, canonical_key, *target, path);
```

正确写法：
```cpp
// ReadBoolAlias 优先读 canonical，不存在则读 alias
*target = ReadBoolAlias(section, canonical_key, alias_key, *target, path);
```

> 教训：2026-06-28 Shield Hide 失效，原因就是精简 config.cpp 时手写了 `ReadBool` 替代 `ReadBoolAlias`。

### 5. 配置热重载

- `config_watcher.cpp` 后台线程每 1 秒检查 INI 文件变化
- 运行时开关切换通过回调中实时读取 `GetConfig()` 实现
- **Hook 安装/卸载不支持热切换**，需重启游戏

### 6. 日志规则

- 所有日志通过 `Log()` 函数输出到 `crimson-desert-tweaks.log`
- 日志文件位于 ASI 同目录
- 生产环境设置 `Verbose=0`，调试时设 `Verbose=1`
- DIAG 前缀用于诊断日志，正常运行时不应出现

### 7. 进度更新规则 ⚠️

**实时更新 PROGRESS.md，不要等功能全部做完才写。**

适用范围：功能开发、Bug 修复、游戏版本适配——任何改变项目状态的编码工作。

更新时机（每完成一个小步骤就写）：

- 逆向分析完一个地址/结构体 → 记录发现
- 写完一段扫描代码 → 标记完成
- 构建测试通过或失败 → 记录结果
- 发现一个问题或限制 → 记录下来
- 修复了一个 Bug → 记录原因和修复方式

写法要求：

- 记录"做了什么、发现了什么、下一步是什么"
- 包含足够的技术细节（地址、签名、结构体偏移、关键决策），下次会话能直接接上

```
❌ 太模糊: "修复了配置读取 bug"
✅ 能接上: "config.cpp ReadBool 别名 bug——HasIniKey 检测 Enable 但 ReadBool 读 Enabled，
          INI 写 Enable=1 时返回 false。已改用 ReadBoolAlias 修复。"
```

**Why:** `PROGRESS.md` 只记录当前项目事实、修改原因、验证结果和下一步，避免把开发过程散落到 README 或对话里。

## 项目路径

- 游戏目录: `D:\steam\steamapps\common\Crimson Desert\bin64\`
- 模组输出目录: `D:\红色沙漠工具\模组\Crimson Desert Tweaks\`
- 逆向参考工具: `D:\红色沙漠工具\修改器\`
- 同类 ASI 模组参考（逆向分析参考）:
  - `D:/AI Agent Resources/GitHub/CrimsonDesertTools-main`
  - `D:/AI Agent Resources/GitHub/CrimsonDesert-player-status-modifier-master`
  - `D:/AI Agent Resources/GitHub/CrimsonDesert-parry-evade-master`
  - `D:/AI Agent Resources/GitHub/CrimsonDesertMods-main`

## 联网操作规则 ⚠️

**所有联网操作（网页搜索、WebFetch、git push/pull、curl、cmake 等联网操作）必须通过本机 VPN 代理。** 用户电脑 VPN 开机自启，对话时 VPN 必定处于开启状态。不经过代理直接联网会导致断联、未响应等问题。

- **HTTP/HTTPS 代理**: `http://127.0.0.1:7890`
- **Git 推送前**临时设置代理：
  ```bash
  git config http.proxy "http://127.0.0.1:7890"
  git push
  git config --unset http.proxy
  ```
- **CMake 下载依赖失败**时，用 GitHub 镜像作为备选：
  ```bash
  git config --global url."<镜像地址>".insteadOf "https://github.com"
  ```

## 强制执行规则（以下规则由项目 Hook 系统强制拦截，不可绕过）

以下规则已配置为 PreToolUse Hook，违反时会被**系统自动拦截**（非 AI 自行判断）。被拦截后请查阅本条规则，**不要尝试绕过去**。

**当前 Hook 部署状态**：`.claude/settings.json` 和 `.claude/hooks/` 已恢复。Hook 规则与下表对应。

如果认为某个规则是误拦截：

1. 停止当前危险操作。
2. 用中文说明：哪个 Hook 拦截、原命令想做什么、为什么可能是误拦截。
3. 询问用户是否允许修改 Hook 或临时放宽规则。
4. 未得到用户明确同意前，不能修改 `.claude/hooks/`、`.claude/settings.json`，也不能换一种写法绕过拦截。

| # | 规则 | Hook 文件 | 说明 |
|---|------|----------|------|
| 1 | **禁止 Write 覆盖已有文件** | `no-overwrite.ps1` | PROGRESS.md、CLAUDE.md、src/\*.cpp/h、CMakeLists.txt 等文件已存在时，禁止使用 Write 覆盖。必须先用 Read 读取内容，再用 Edit 在已有内容基础上追加或修改。**绝不能覆盖已有内容。** |
| 2 | **禁止修改/删除 deps/safetyhook/** | `safetyhook-guard.ps1` | 这是 Hook 框架完整源码，构建必须，无例外。 |
| 3 | **联网操作必须通过 VPN 代理** | `proxy-check.ps1` | 所有 git push/pull、curl、cmake 等联网操作必须设代理 `127.0.0.1:7890`，否则拦截。 |
| 4 | **构建产物不自动复制** | `no-copy-build.ps1` | build/Release/ 下的产物留在原处，用户通过 DMM 手动安装 ASI 模组。AI 不负责复制。 |
| 5 | **禁止危险 Git 还原** | `destructive-git-guard.ps1` | 禁止 `git reset --hard`、`git checkout -- .`、`git clean -fd` 等会丢失未提交改动的命令。 |
| 6 | **禁止写入游戏目录** | `game-dir-guard.ps1` | 禁止复制、移动、删除、写入 `D:\steam\steamapps\common\Crimson Desert\bin64\`。 |
| 7 | **修改源码后提醒更新 PROGRESS.md** | `progress-reminder.ps1` | 修改 src/\*.cpp/h 后弹出提醒（软提醒，不拦截）。 |

## 已知问题和注意事项

1. **不要删除 deps/safetyhook/** — 这是 Hook 框架的完整源码，构建必须。（已由 Hook #2 强制执行）
