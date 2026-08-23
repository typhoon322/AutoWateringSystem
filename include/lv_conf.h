#pragma once
#define LV_CONF_H

// BOARD_HAS_LVGL comes from the build flags (-DBOARD_HAS_LVGL), not from a board
// header. Do NOT include boards/*.h here: LVGL compiles its own .c files as C,
// and board headers use C++ (constexpr), which breaks the LVGL build.

#if BOARD_HAS_LVGL

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

#define LV_USE_LOG 0

#define LV_TICK_CUSTOM 0
#define LV_DEF_REFR_PERIOD 33

#define LV_DPI_DEF 130

#define LV_USE_OS LV_OS_NONE

#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_LIST 1
#define LV_USE_ARC 1
#define LV_USE_BAR 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 1
#define LV_USE_ROLLER 0
#define LV_USE_SLIDER 0
#define LV_USE_SWITCH 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_MSGBOX 0
#define LV_USE_KEYBOARD 0
#define LV_USE_TEXTAREA 1

// LVGL 9 lvgl.h 无条件包含 spinbox/spinner/calendar 头，其依赖 ARC/TEXTAREA/
// DROPDOWN 必须开启，否则 #error。本工程 UI 实际用不到这三个控件，仅需满足头文件依赖。

#define LV_USE_FLOAT 0

#endif
