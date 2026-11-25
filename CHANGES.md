# Changes against ToDoList 7.0.11

Baseline: upstream `todolist_src7.0.11_VC2010` (VC2010 project files, 2021-11-14).
Complete unified diff: `patches/0001-todolist-7011-modifications.patch` (text + binary).

Totals: **173 files** changed, **+31005 / -874 lines** (6 binary).

Two kinds of change are mixed in this set:

1. **Toolchain port** – VC2010 → VC2022: message-map signatures (`LONG`→`LRESULT`, `BOOL`→`INT_PTR`), project files switched to v143 toolset, `/O2` and large-address-aware for Win32 Release.
2. **Functional changes** – calendar/gantt layout, tray icon lifecycle, window instance lookup, load-time performance, filter bar, list/tree redraw, and a handful of behavioural fixes. See `NOTICE.md`.

## 3rdParty

| File | + | - |
|---|---|---|
| `3rdParty/3rdParty.vcxproj` | 95 | 3 |
| `3rdParty/CalendarCtrl.cpp` | 137 | 61 |
| `3rdParty/CalendarCtrl.h` | 14 | 1 |
| `3rdParty/ColourPicker.cpp` | 3 | 3 |
| `3rdParty/ColourPicker.h` | 3 | 3 |
| `3rdParty/Detours/Detours.dsp` | 155 | 0 |
| `3rdParty/Detours/Makefile` | 119 | 0 |
| `3rdParty/Detours/creatwth.cpp` | 1584 | 0 |
| `3rdParty/Detours/detours.cpp` | 2368 | 0 |
| `3rdParty/Detours/detours.h` | 1098 | 0 |
| `3rdParty/Detours/detver.h` | 27 | 0 |
| `3rdParty/Detours/disasm.cpp` | 4236 | 0 |
| `3rdParty/Detours/disolarm.cpp` | 2 | 0 |
| `3rdParty/Detours/disolarm64.cpp` | 2 | 0 |
| `3rdParty/Detours/disolia64.cpp` | 2 | 0 |
| `3rdParty/Detours/disolx64.cpp` | 2 | 0 |
| `3rdParty/Detours/disolx86.cpp` | 2 | 0 |
| `3rdParty/Detours/image.cpp` | 2161 | 0 |
| `3rdParty/Detours/modules.cpp` | 929 | 0 |
| `3rdParty/Detours/strsafe.cpp` | 466 | 0 |
| `3rdParty/Detours/strsafe.h` | 32 | 0 |
| `3rdParty/Detours/uimports.cpp` | 269 | 0 |
| `3rdParty/FPSMiniCalendarCtrl.cpp` | 13 | 5 |
| `3rdParty/FPSMiniCalendarCtrl.h` | 3 | 3 |
| `3rdParty/ProgressCtrlWithTimer.cpp` | 1 | 1 |
| `3rdParty/ProgressCtrlWithTimer.h` | 1 | 1 |
| `3rdParty/XPTabCtrl.cpp` | 3 | 1 |
| `3rdParty/sha2.cpp` | 3 | 1 |

## CalendarExt

| File | + | - |
|---|---|---|
| `CalendarExt/CalendarExt.vcxproj` | 166 | 9 |
| `CalendarExt/CalendarWnd.cpp` | 180 | 28 |
| `CalendarExt/CalendarWnd.h` | 14 | 1 |
| `CalendarExt/TaskCalendarCtrl.cpp` | 440 | 150 |
| `CalendarExt/TaskCalendarCtrl.h` | 25 | 6 |
| `CalendarExt/calprof.h` | 41 | 0 |
| `CalendarExt/resource.h` | 2 | 1 |

## EncryptDecrypt

| File | + | - |
|---|---|---|
| `EncryptDecrypt/EncryptDecrypt.vcxproj` | 157 | 8 |

## FMindImportExport

| File | + | - |
|---|---|---|
| `FMindImportExport/FMindImportExport.vcxproj` | 153 | 6 |

## FtpStorage

| File | + | - |
|---|---|---|
| `FtpStorage/FtpStorage.vcxproj` | 156 | 6 |

## GPExport

| File | + | - |
|---|---|---|
| `GPExport/GPExport.vcxproj` | 157 | 6 |

## GanttChartExt

| File | + | - |
|---|---|---|
| `GanttChartExt/GanttChartExt.vcxproj` | 161 | 6 |
| `GanttChartExt/GanttCreateDependsDlg.h` | 1 | 1 |
| `GanttChartExt/GanttTreeListCtrl.cpp` | 34 | 14 |
| `GanttChartExt/ganttprof.h` | 41 | 0 |

## MLOImport

| File | + | - |
|---|---|---|
| `MLOImport/MLOImport.vcxproj` | 154 | 6 |

## MySpellCheck

| File | + | - |
|---|---|---|
| `MySpellCheck/MySpellCheck.vcxproj` | 146 | 4 |

## OdbcStorage

| File | + | - |
|---|---|---|
| `OdbcStorage/OdbcStorage.vcxproj` | 156 | 6 |

## OutlookImpExp

| File | + | - |
|---|---|---|
| `OutlookImpExp/OutlookImpExp.vcxproj` | 154 | 6 |
| `OutlookImpExp/OutlookImportDlg.cpp` | 2 | 2 |
| `OutlookImpExp/OutlookImportDlg.h` | 1 | 1 |

## PlainTextImport

| File | + | - |
|---|---|---|
| `PlainTextImport/PlainTextImport.vcxproj` | 154 | 6 |

## RTFContentCtrl

| File | + | - |
|---|---|---|
| `RTFContentCtrl/RTFContentCtrl.vcxproj` | 165 | 8 |

## Shared

| File | + | - |
|---|---|---|
| `Shared/AboutDlg.cpp` | 1 | 1 |
| `Shared/AboutDlg.h` | 1 | 1 |
| `Shared/ChineseTable.h` | 1749 | 0 |
| `Shared/DatabaseEx.cpp` | 1 | 1 |
| `Shared/DatabaseEx.h` | 1 | 1 |
| `Shared/DateHelperEnums.h` | 100 | 0 |
| `Shared/EnMenu.cpp` | 2 | 2 |
| `Shared/EnMenu.h` | 1 | 1 |
| `Shared/FolderDialog.cpp` | 1 | 1 |
| `Shared/FolderDialog.h` | 1 | 1 |
| `Shared/GraphicsMisc.cpp` | 3 | 3 |
| `Shared/Icon.cpp` | 72 | 0 |
| `Shared/Icon.h` | 34 | 0 |
| `Shared/InputListCtrl.cpp` | 2 | 2 |
| `Shared/InputListCtrl.h` | 2 | 2 |
| `Shared/Localizer.cpp` | 55 | 20 |
| `Shared/MenuIconMgr.cpp` | 1 | 3 |
| `Shared/Misc.cpp` | 5 | 0 |
| `Shared/RoundCorner.h` | 9 | 0 |
| `Shared/RuntimeDlg.h` | 1 | 1 |
| `Shared/Shared.vcxproj` | 106 | 4 |
| `Shared/Subclass.cpp` | 2 | 2 |
| `Shared/TRAYICON.CPP` | 350 | 5 |
| `Shared/TRAYICON.H` | 14 | 1 |
| `Shared/TransTextMgr.cpp` | 40 | 0 |
| `Shared/TransTextMgr.h` | 1 | 0 |
| `Shared/TreeDragDropHelper.cpp` | 2 | 2 |
| `Shared/TreeDragDropHelper.h` | 3 | 3 |
| `Shared/WorkingWeek.h` | 205 | 0 |
| `Shared/autocombobox.cpp` | 136 | 4 |
| `Shared/autocombobox.h` | 7 | 0 |
| `Shared/delayredraw.cpp` | 1 | 1 |
| `Shared/delayredraw.h` | 1 | 1 |
| `Shared/encolordialog.cpp` | 1 | 1 |
| `Shared/encolordialog.h` | 1 | 1 |
| `Shared/enlistctrl.cpp` | 1 | 1 |
| `Shared/enlistctrl.h` | 1 | 1 |
| `Shared/entoolbar.cpp` | 41 | 0 |
| `Shared/entoolbar.h` | 1 | 1 |
| `Shared/ownerdrawcomboboxbase.cpp` | 4 | 7 |
| `Shared/passworddialog.cpp` | 1 | 1 |
| `Shared/passworddialog.h` | 1 | 1 |
| `Shared/richeditbasectrl.cpp` | 6 | 6 |
| `Shared/richeditbasectrl.h` | 3 | 3 |
| `Shared/urlricheditctrl.cpp` | 1 | 1 |
| `Shared/urlricheditctrl.h` | 1 | 1 |

## StatisticsExt

| File | + | - |
|---|---|---|
| `StatisticsExt/StatisticsExt.vcxproj` | 158 | 6 |
| `StatisticsExt/StatisticsWnd.cpp` | 13 | 8 |

## TDLTest

| File | + | - |
|---|---|---|
| `TDLTest/TDLTest.vcxproj` | 148 | 10 |

## TDLUninstall

| File | + | - |
|---|---|---|
| `TDLUninstall/TDLUninstall.vcxproj` | 146 | 5 |

## TDLUpdate

| File | + | - |
|---|---|---|
| `TDLUpdate/TDLUpdate.vcxproj` | 148 | 5 |

## ToDoList

| File | + | - |
|---|---|---|
| `ToDoList/FilteredToDoCtrl.cpp` | 1 | 1 |
| `ToDoList/FilteredToDoCtrl.h` | 1 | 1 |
| `ToDoList/PreferencesGenPage.cpp` | 6 | 26 |
| `ToDoList/PreferencesGenPage.h` | 1 | 5 |
| `ToDoList/TDCTaskCtrlBase.cpp` | 209 | 22 |
| `ToDoList/TDCTaskCtrlBase.h` | 11 | 2 |
| `ToDoList/TDCTaskListCtrl.cpp` | 5 | 0 |
| `ToDoList/TDCTaskListCtrl.h` | 2 | 0 |
| `ToDoList/TDCTaskTreeCtrl.cpp` | 89 | 12 |
| `ToDoList/TDCTaskTreeCtrl.h` | 12 | 0 |
| `ToDoList/TDCViewTabControl.cpp` | 5 | 3 |
| `ToDoList/TDLAboutDlg.cpp` | 5 | 1 |
| `ToDoList/TDLFilterBar.cpp` | 24 | 8 |
| `ToDoList/TDLFilterBar.h` | 3 | 1 |
| `ToDoList/TDLFindTasksDlg.h` | 1 | 1 |
| `ToDoList/TDLPrefMigrationDlg.h` | 1 | 1 |
| `ToDoList/TabbedToDoCtrl.cpp` | 19 | 19 |
| `ToDoList/TaskFile.cpp` | 55 | 5 |
| `ToDoList/TaskFile.h` | 3 | 0 |
| `ToDoList/TaskSelectionDlg.h` | 1 | 1 |
| `ToDoList/ToDoCtrl.cpp` | 397 | 37 |
| `ToDoList/ToDoCtrl.h` | 14 | 1 |
| `ToDoList/ToDoCtrlData.cpp` | 1 | 1 |
| `ToDoList/ToDoCtrlFind.cpp` | 124 | 0 |
| `ToDoList/ToDoCtrlFind.h` | 18 | 0 |
| `ToDoList/ToDoCtrlReminders.cpp` | 1 | 1 |
| `ToDoList/ToDoCtrlReminders.h` | 1 | 1 |
| `ToDoList/ToDoList.cpp` | 146 | 54 |
| `ToDoList/ToDoList.rc` | 7 | 6 |
| `ToDoList/ToDoList.vcxproj` | 153 | 7 |
| `ToDoList/ToDoListWnd.cpp` | 501 | 154 |
| `ToDoList/ToDoListWnd.h` | 7 | 2 |
| `ToDoList/ToDoList_All.sln` | 119 | 2 |
| `ToDoList/res/task_tray.ico` | - | - |
| `ToDoList/resource.h` | 3 | 1 |
| `ToDoList/tdcstruct.h` | 26 | 0 |
| `ToDoList/tdlprof.h` | 55 | 0 |

## TransText

| File | + | - |
|---|---|---|
| `TransText/TransText.vcxproj` | 157 | 6 |

## WorkloadExt

| File | + | - |
|---|---|---|
| `WorkloadExt/EditAllocationsDlg.cpp` | 77 | 0 |
| `WorkloadExt/EditAllocationsDlg.h` | 52 | 0 |
| `WorkloadExt/EditAllocationsListCtrl.cpp` | 178 | 0 |
| `WorkloadExt/EditAllocationsListCtrl.h` | 63 | 0 |
| `WorkloadExt/Resource.h` | 91 | 0 |
| `WorkloadExt/StdAfx.cpp` | 8 | 0 |
| `WorkloadExt/StdAfx.h` | 53 | 0 |
| `WorkloadExt/WorkloadChart.cpp` | 308 | 0 |
| `WorkloadExt/WorkloadChart.h` | 64 | 0 |
| `WorkloadExt/WorkloadCtrl.cpp` | 2988 | 0 |
| `WorkloadExt/WorkloadCtrl.h` | 216 | 0 |
| `WorkloadExt/WorkloadEnum.h` | 82 | 0 |
| `WorkloadExt/WorkloadExt.cpp` | 112 | 0 |
| `WorkloadExt/WorkloadExt.dsp` | 239 | 0 |
| `WorkloadExt/WorkloadExt.h` | 51 | 0 |
| `WorkloadExt/WorkloadExt.rc` | 473 | 0 |
| `WorkloadExt/WorkloadExt.vcxproj` | 351 | 0 |
| `WorkloadExt/WorkloadExt.vcxproj.filters` | 116 | 0 |
| `WorkloadExt/WorkloadMsg.h` | 42 | 0 |
| `WorkloadExt/WorkloadPreferencesDlg.cpp` | 268 | 0 |
| `WorkloadExt/WorkloadPreferencesDlg.h` | 126 | 0 |
| `WorkloadExt/WorkloadStatic.h` | 37 | 0 |
| `WorkloadExt/WorkloadStruct.cpp` | 1041 | 0 |
| `WorkloadExt/WorkloadStruct.h` | 275 | 0 |
| `WorkloadExt/WorkloadWnd.cpp` | 1196 | 0 |
| `WorkloadExt/WorkloadWnd.h` | 171 | 0 |
| `WorkloadExt/res/Workload.ico` | - | - |
| `WorkloadExt/res/WorkloadExt.rc2` | 13 | 0 |
| `WorkloadExt/res/XPcheckboxes.bmp` | - | - |
| `WorkloadExt/res/help_button.ico` | - | - |
| `WorkloadExt/res/toolbar.bmp` | - | - |
| `WorkloadExt/res/toolbar_std.bmp` | - | - |

## iCalImportExport

| File | + | - |
|---|---|---|
| `iCalImportExport/iCalImportExport.vcxproj` | 154 | 6 |



## Gantt full-rebuild list sync optimization

- GanttChartExt/GanttTreeListCtrl.cpp/.h: on a full task-tree rebuild the list
  control is cleared first, so no item can already be present. The generic
  sync path still called ListView_FindItem() once per item, scanning the
  growing list (O(N^2)) only to get -1. Added ExpandListAppendOnly() used
  only by the full-rebuild path; it appends directly with identical result.
  Incremental expand/collapse paths are unchanged.

## Post-publication sync (2026-11-26)

The patch above was generated from the state at 02:44. A later byte-for-byte
audit against the working tree found 14 source files that were either not yet
published or had changed afterwards; they are now included in this repository
as full source files. The patch is kept as-is for traceability; the authoritative
source of these 14 files is the file content in the tree.

Newly published (6):
`GanttChartExt/GanttTreeListCtrl.h`, `Shared/HoldRedraw.cpp`,
`Shared/TreeListSyncer.cpp`, `ToDoList/TDLAboutDlg.h`,
`ToDoList/TabbedToDoCtrl.h`, `Shared/DrawingCache.h`

Updated to current working-tree content (8):
`GanttChartExt/GanttTreeListCtrl.cpp`, `ToDoList/TDLAboutDlg.cpp`,
`ToDoList/TabbedToDoCtrl.cpp`, `ToDoList/ToDoCtrl.cpp`,
`ToDoList/ToDoList.rc`, `ToDoList/ToDoListWnd.cpp`,
`ToDoList/ToDoListWnd.h`, `ToDoList/resource.h`

These changes cover the calendar/gantt layout fix, the task-edit/filter-bar
collapse, the list-view removal, the tray-icon lifecycle rewrite, the window
instance lookup by class name, the About dialog, and the associated resource IDs.
