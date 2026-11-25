# TaskList CalendarExt — modified source

This repository publishes the **modified source code** of the `CalendarExt` plugin
(and the two `3rdParty/CalendarCtrl` files it builds on) used by an in-house TaskList
deployment. It exists to satisfy the source-availability obligation
of the **Eclipse Public License 1.0 (EPL-1.0)** under which ToDoList is distributed.

---

## 1. What this is

| Item | Value |
|---|---|
| Upstream project | ToDoList by AbstractSpoon |
| Upstream repository | https://github.com/abstractspoon/ToDoList_Dev |
| Upstream licence | Eclipse Public License 1.0 (see `LICENSE`) |
| Baseline of this port | ToDoList **7.0.11**, VC2010 project files |
| Baseline archive | `todolist_src7.0.11_VC2010` (internal snapshot, 2021-11-14) |
| Files changed | 173 (see `CHANGES.md`) |
| Nature of changes | VC2010 to VC2022 port, plus calendar/gantt layout, tray icon, instance handling, load-time performance |

Everything in this repository is derived from EPL-1.0 code and remains under
EPL-1.0. No file here is closed-source.

## 2. What is in the box

Only files that differ from the 7.0.11 baseline are published, but **all** of
them are here, not just the calendar plugin:

```
CalendarExt/            calendar plugin: layout, per-row scrolling, painting,
                        collapsible filter bar, redraw cache
3rdParty/               CalendarCtrl, FPS mini calendars, ColourPicker,
                        ProgressCtrlWithTimer, XPTabCtrl, sha2
Shared/                 tray icon, auto-complete combo, delay-redraw,
                        tree drag/drop, Misc::Atof, toolbars, colour dialog ...
ToDoList/               main application: task tree/list controllers, calendar
                        tabs, find dialog, filter bar, preferences, resources
GanttChartExt/          gantt plugin
StatisticsExt/          statistics plugin
OutlookImpExp/ iCalImportExport/ GPExport/ MLOImport/ PlainTextImport/
FtpStorage/ OdbcStorage/ RTFContentCtrl/ MySpellCheck/ EncryptDecrypt/
TransText/ FMindImportExport/ TDLTest/ TDLUninstall/ TDLUpdate/
                        project files only (v143 toolset port)
WorkloadExt/ 3rdParty/Detours/
                        upstream files absent from the 7.0.11 archive; kept for
                        completeness, not built by this deployment
patches/
   0001-todolist-7011-modifications.patch
                        unified diff (text + binary) of every change
CHANGES.md              itemised list with +/- line counts, grouped by module
NOTICE.md               originator / modification statement (EPL-1.0 section 3)
LICENSE                 Eclipse Public License 1.0, full text
```

Files that were **not** touched are not duplicated here. To obtain a complete,
buildable tree, take the upstream ToDoList 7.0.11 source, overwrite it with the
files from this repository (or apply the patch), then build `ToDoList_All.sln`
with `Unicode Release` / `Win32`.

## 3. Building

The plugin links against the rest of the ToDoList solution
(`Shared`, `Interfaces`, `3rdParty`, `Lib`), so a standalone build of this
folder is not possible. Inside a complete 7.0.11 tree:

1. Build `3rdParty` (static library) first — `CalendarCtrl.cpp` lives there.
2. Build `CalendarExt` — `Unicode Release`, `Win32`.
3. Output is `Bin_Release/CalendarExt.dll`, loaded by `TaskList.exe`.

`CalendarCtrl.cpp` is compiled into `Lib/3rdParty.lib`; if you change it you
must rebuild that library **before** relinking `CalendarExt`, otherwise the
DLL silently keeps the old code.

## 4. Licence

All source files here are governed by the Eclipse Public License 1.0.
See `LICENSE` for the full text and `NOTICE.md` for the originator statement.

ToDoList itself is Copyright (c) AbstractSpoon and contributors.
The modifications published here are contributed under the same EPL-1.0
terms.

## 5. Contact

Issues and questions about these modifications: please open an issue in this
repository. Issues about ToDoList itself belong upstream.

---

## 中文说明

本仓库是生产环境中的 TaskList 所使用的 **CalendarExt 日历插件修改版源码**，
按 ToDoList 所采用的 **Eclipse Public License 1.0** 第 3 条的要求公开。

- 上游：AbstractSpoon ToDoList 7.0.11（VC2010 工程）
- 本次公开的是**被修改的文件**，共 7 个，清单见 `CHANGES.md`
- 完整可编译工程 = 上游 7.0.11 源码 + 本仓库文件（或 `patches/` 下的补丁）
- 修改内容为日历界面布局、按行滚动、绘制与重绘开销、过滤栏折叠，
  不包含任何业务数据、数据库连接或客户信息

许可证全文见 `LICENSE`。修改部分同样以 EPL-1.0 授权。
