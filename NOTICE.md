# NOTICE

## Originator

All files in this repository derive from **ToDoList**, Copyright (c) AbstractSpoon
and the ToDoList contributors, distributed under the Eclipse Public License 1.0.

Upstream: https://github.com/abstractspoon/ToDoList_Dev

## Modifications

The files listed in `CHANGES.md` were modified for an in-house production
deployment of TaskList (a ToDoList build), starting from the ToDoList 7.0.11
source and VC2010 project files.

The set contains two kinds of change:

1. Toolchain port from VC2010 to VC2022: message-map signatures (`LONG` to
   `LRESULT`, `BOOL` to `INT_PTR`), project files retargeted to the v143 toolset,
   `/O2` and large-address-aware enabled for the Win32 release build.
2. Functional changes: calendar and gantt view layout, per-row scrolling,
   painting and redraw cost, tray icon lifecycle, single-instance window lookup,
   task tree/list selection handling, load-time performance, filter bar and task
   edit panel collapsing.

They contain no business data, no database access code and no customer
information.

## Licence of the modifications

The modifications are contributed under the **Eclipse Public License 1.0**, the
same licence as the original files. They are not dual-licensed and no additional
restrictions are placed on them.

Full licence text: `LICENSE`.

## Source availability

Per EPL-1.0 §3, the source code of the binary shipped to third parties is
available in this repository, including all modifications.

## Third-party components

`CalendarExt` links against other ToDoList components and third-party libraries
that are **not** covered by this repository; their own licences apply
(see the upstream project for details). Only the files listed in `CHANGES.md`
are published here.
