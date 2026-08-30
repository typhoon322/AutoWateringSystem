/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --no-compress --font NotoSansSC-Regular.otf -r 0x20-0x7E --symbols 盆浇水开关自动故障恢复返回干转超时限额今日泵阀下限上限水量正在停止清除灌溉中网络连接正常偏干偏湿无效空闲完成检查锁定未知度保护已， --size 14 --bpp 1 --format lvgl --force-fast-kern-format -o /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem/src/ui/lv_font_cn_14.c
 ******************************************************************************/

#if BOARD_HAS_LVGL
#include "lvgl.h"

#ifndef LV_FONT_CN_14
#define LV_FONT_CN_14 1
#endif

#if LV_FONT_CN_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xaa, 0xa8, 0x3c,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x49, 0x24, 0xbf, 0x49, 0x24, 0xbf, 0x52, 0x49,
    0x0,

    /* U+0024 "$" */
    0x23, 0xa3, 0x8, 0x61, 0xc3, 0x8, 0x62, 0xe2,
    0x0,

    /* U+0025 "%" */
    0x70, 0x8d, 0x90, 0x89, 0x8, 0xa0, 0x8a, 0xed,
    0xdb, 0x75, 0x10, 0x91, 0x9, 0x10, 0x9b, 0x10,
    0xe0,

    /* U+0026 "&" */
    0x38, 0x22, 0x11, 0x9, 0x87, 0x3, 0x7, 0xc5,
    0x32, 0x8e, 0x67, 0x9e, 0x60,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x29, 0x29, 0x24, 0x92, 0x24, 0x88,

    /* U+0029 ")" */
    0x89, 0x22, 0x49, 0x24, 0xa4, 0xa0,

    /* U+002A "*" */
    0x25, 0x1c, 0xe1, 0x0,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0xf6, 0x0,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x42, 0x21, 0x8, 0x84, 0x22, 0x10, 0x84,
    0x40,

    /* U+0030 "0" */
    0x31, 0x28, 0x61, 0x86, 0x18, 0x61, 0x85, 0x27,
    0x0,

    /* U+0031 "1" */
    0x63, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x8f,
    0xc0,

    /* U+0032 "2" */
    0x7b, 0x30, 0x41, 0x4, 0x20, 0x84, 0x21, 0x8f,
    0xc0,

    /* U+0033 "3" */
    0x78, 0x88, 0x10, 0x20, 0xc7, 0x1, 0x1, 0x3,
    0x8d, 0xf0,

    /* U+0034 "4" */
    0xc, 0x18, 0x51, 0xa2, 0x4c, 0xb1, 0x7f, 0x4,
    0x8, 0x10,

    /* U+0035 "5" */
    0x7c, 0x81, 0x2, 0x7, 0xc8, 0xc0, 0x81, 0x3,
    0x89, 0xe0,

    /* U+0036 "6" */
    0x39, 0x8, 0x20, 0xbb, 0x38, 0x61, 0x85, 0x33,
    0x80,

    /* U+0037 "7" */
    0xfc, 0x20, 0x84, 0x10, 0x42, 0x8, 0x20, 0x82,
    0x0,

    /* U+0038 "8" */
    0x39, 0x14, 0x51, 0x28, 0xe4, 0xe1, 0x87, 0x37,
    0x80,

    /* U+0039 "9" */
    0x73, 0x28, 0x61, 0x87, 0x37, 0x41, 0x8, 0x27,
    0x0,

    /* U+003A ":" */
    0xf0, 0xf,

    /* U+003B ";" */
    0xf0, 0xf, 0x60,

    /* U+003C "<" */
    0x2, 0x1c, 0xc6, 0x6, 0x3, 0x80, 0x80,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x1, 0xc0, 0xe0, 0x31, 0xcc, 0x20, 0x0,

    /* U+003F "?" */
    0x79, 0x10, 0x41, 0x8, 0x43, 0xc, 0x0, 0xc3,
    0x0,

    /* U+0040 "@" */
    0xf, 0x83, 0x6, 0x60, 0x24, 0x1, 0xc7, 0x18,
    0x91, 0x91, 0x19, 0x11, 0x93, 0x2c, 0xdc, 0x40,
    0x2, 0x0, 0x1f, 0x0,

    /* U+0041 "A" */
    0x18, 0x18, 0x14, 0x34, 0x24, 0x26, 0x62, 0x7e,
    0x43, 0x41, 0xc1,

    /* U+0042 "B" */
    0xf9, 0x1a, 0x14, 0x28, 0xdf, 0x21, 0xc1, 0x83,
    0xf, 0xf0,

    /* U+0043 "C" */
    0x3c, 0xc5, 0x4, 0x8, 0x10, 0x20, 0x40, 0x40,
    0xc4, 0xf0,

    /* U+0044 "D" */
    0xf9, 0x1a, 0x14, 0x18, 0x30, 0x60, 0xc1, 0x85,
    0x1b, 0xe0,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x83, 0xe8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xe8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0047 "G" */
    0x1e, 0x61, 0x40, 0x80, 0x80, 0x87, 0x81, 0x81,
    0x41, 0x61, 0x1e,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x18, 0x3f, 0xe0, 0xc1, 0x83,
    0x6, 0x8,

    /* U+0049 "I" */
    0xff, 0xe0,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x84, 0x21, 0xc, 0x5c,

    /* U+004B "K" */
    0x85, 0x1a, 0x64, 0x8b, 0x1e, 0x36, 0x44, 0x8d,
    0xa, 0x8,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+004D "M" */
    0xc1, 0xe0, 0xf0, 0x74, 0x5a, 0x2d, 0xb6, 0x53,
    0x29, 0x98, 0xc4, 0x60, 0x20,

    /* U+004E "N" */
    0x83, 0x87, 0xd, 0x1b, 0x32, 0x66, 0xc5, 0x87,
    0xe, 0x8,

    /* U+004F "O" */
    0x3c, 0x31, 0x90, 0x50, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x41, 0x31, 0x8f, 0x80,

    /* U+0050 "P" */
    0xf9, 0xe, 0xc, 0x18, 0x30, 0xbe, 0x40, 0x81,
    0x2, 0x0,

    /* U+0051 "Q" */
    0x3e, 0x31, 0x90, 0x50, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x41, 0x1b, 0x87, 0x1, 0x80, 0x38,

    /* U+0052 "R" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa6, 0x46, 0x85,
    0xe, 0x8,

    /* U+0053 "S" */
    0x7a, 0x18, 0x20, 0xe0, 0xe0, 0xc1, 0x6, 0x37,
    0x80,

    /* U+0054 "T" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,
    0x88, 0xe0,

    /* U+0056 "V" */
    0xc1, 0xc3, 0x42, 0x42, 0x66, 0x24, 0x24, 0x24,
    0x1c, 0x18, 0x18,

    /* U+0057 "W" */
    0xc6, 0x14, 0x63, 0x46, 0x24, 0x72, 0x65, 0x26,
    0x92, 0x29, 0x62, 0x9c, 0x28, 0xc3, 0xc, 0x30,
    0xc0,

    /* U+0058 "X" */
    0x42, 0x66, 0x24, 0x34, 0x18, 0x18, 0x18, 0x24,
    0x24, 0x42, 0x43,

    /* U+0059 "Y" */
    0xc2, 0x89, 0x11, 0x42, 0x85, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+005A "Z" */
    0xfe, 0x8, 0x30, 0x41, 0x82, 0x8, 0x10, 0x41,
    0x3, 0xf8,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x49, 0xc0,

    /* U+005C "\\" */
    0x82, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10,
    0x84,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x93, 0xc0,

    /* U+005E "^" */
    0x30, 0xc5, 0x12, 0x4a, 0x10,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0x19, 0x90,

    /* U+0061 "a" */
    0x78, 0x10, 0x4f, 0xc6, 0x18, 0xdd,

    /* U+0062 "b" */
    0x82, 0x8, 0x2e, 0xce, 0x18, 0x61, 0x86, 0x2f,
    0x0,

    /* U+0063 "c" */
    0x39, 0x8, 0x20, 0x82, 0x4, 0xe,

    /* U+0064 "d" */
    0x2, 0x4, 0x9, 0xd4, 0x30, 0x60, 0xc1, 0x82,
    0x8c, 0xe8,

    /* U+0065 "e" */
    0x39, 0x38, 0x7f, 0x82, 0x4, 0xe,

    /* U+0066 "f" */
    0x3a, 0x11, 0xe4, 0x21, 0x8, 0x42, 0x10,

    /* U+0067 "g" */
    0x7f, 0xa, 0x16, 0x67, 0x90, 0x20, 0x3e, 0x83,
    0xd, 0xf0,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0x9f, 0xe0,

    /* U+006A "j" */
    0x20, 0x12, 0x49, 0x24, 0x93, 0x80,

    /* U+006B "k" */
    0x82, 0x8, 0x22, 0x9a, 0xca, 0x34, 0x9a, 0x28,
    0xc0,

    /* U+006C "l" */
    0xaa, 0xaa, 0xac,

    /* U+006D "m" */
    0xb9, 0xd8, 0xc6, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x61, 0xc, 0x21,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61,

    /* U+006F "o" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x51, 0x1c,

    /* U+0070 "p" */
    0xbb, 0x38, 0x61, 0x86, 0x18, 0xbc, 0x82, 0x8,
    0x0,

    /* U+0071 "q" */
    0x3e, 0x86, 0xc, 0x18, 0x30, 0x51, 0x9d, 0x2,
    0x4, 0x8,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88, 0x88,

    /* U+0073 "s" */
    0x74, 0x20, 0xc1, 0x86, 0x3e,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x44, 0x43,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0xdd,

    /* U+0076 "v" */
    0xc2, 0x8d, 0x13, 0x22, 0xc5, 0xe, 0xc,

    /* U+0077 "w" */
    0xc4, 0x68, 0xc9, 0x29, 0x35, 0x22, 0xa4, 0x53,
    0xc, 0x61, 0x8c,

    /* U+0078 "x" */
    0x44, 0xd8, 0xa1, 0xc3, 0x85, 0x13, 0x62,

    /* U+0079 "y" */
    0xc2, 0x8d, 0x11, 0x22, 0xc5, 0x6, 0xc, 0x10,
    0x61, 0x80,

    /* U+007A "z" */
    0x7c, 0x21, 0x84, 0x31, 0x84, 0x3f,

    /* U+007B "{" */
    0x69, 0x24, 0x94, 0x49, 0x24, 0xc0,

    /* U+007C "|" */
    0xff, 0xff,

    /* U+007D "}" */
    0xc9, 0x24, 0x91, 0x49, 0x25, 0x80,

    /* U+007E "~" */
    0xe4, 0x70,

    /* U+4E0A "上" */
    0x2, 0x0, 0x10, 0x0, 0x80, 0x4, 0x0, 0x20,
    0x1, 0xf8, 0x8, 0x0, 0x40, 0x2, 0x0, 0x10,
    0x0, 0x81, 0xff, 0xf0,

    /* U+4E0B "下" */
    0xff, 0xf0, 0x40, 0x4, 0x0, 0x60, 0x7, 0x0,
    0x4c, 0x4, 0x20, 0x40, 0x4, 0x0, 0x40, 0x4,
    0x0, 0x40,

    /* U+4E2D "中" */
    0x4, 0x0, 0x80, 0x10, 0x7f, 0xf8, 0x43, 0x8,
    0x61, 0xc, 0x21, 0xff, 0xf0, 0x84, 0x10, 0x2,
    0x0, 0x40,

    /* U+4ECA "今" */
    0x1, 0x0, 0xc, 0x0, 0x48, 0x6, 0x18, 0x32,
    0x31, 0x84, 0x20, 0x0, 0xf, 0xfc, 0x0, 0x20,
    0x1, 0x0, 0xc, 0x0, 0x60, 0x3, 0x0,

    /* U+4FDD "保" */
    0x0, 0x0, 0xbf, 0x8d, 0x4, 0x48, 0x26, 0x7f,
    0x50, 0x42, 0x82, 0x5, 0xff, 0x21, 0xc1, 0x1d,
    0x9, 0xac, 0x59, 0x32, 0x8, 0x0,

    /* U+504F "偏" */
    0x0, 0x0, 0x84, 0xb, 0xfe, 0x50, 0x16, 0xff,
    0xf4, 0x2, 0xaf, 0xe5, 0x55, 0x2a, 0xa9, 0x5f,
    0xca, 0xaa, 0x65, 0x52, 0x2b, 0x80,

    /* U+505C "停" */
    0x10, 0x80, 0xff, 0xcc, 0x0, 0x4f, 0xe6, 0x41,
    0x33, 0xfa, 0x80, 0x5, 0xff, 0x28, 0x9, 0x3f,
    0x88, 0x20, 0x41, 0x2, 0x38, 0x0,

    /* U+5173 "关" */
    0x20, 0x41, 0x8, 0x10, 0x87, 0xfe, 0x6, 0x0,
    0x60, 0x6, 0xf, 0xff, 0x6, 0x0, 0xd0, 0x19,
    0x83, 0xc, 0xc0, 0x30,

    /* U+52A8 "动" */
    0x0, 0x8f, 0xc8, 0x0, 0x80, 0x3f, 0x3, 0xff,
    0xc9, 0x20, 0x94, 0x89, 0x48, 0x94, 0x51, 0xfd,
    0x18, 0x31, 0x2, 0xe0,

    /* U+56DE "回" */
    0xff, 0xf8, 0x1, 0x80, 0x19, 0xf9, 0x90, 0x99,
    0x9, 0x90, 0x99, 0xf9, 0x80, 0x18, 0x1, 0xff,
    0xf8, 0x1,

    /* U+5728 "在" */
    0x8, 0x0, 0x80, 0xff, 0xf1, 0x0, 0x11, 0x2,
    0x10, 0x61, 0xe, 0xfe, 0xa1, 0x2, 0x10, 0x21,
    0x2, 0x10, 0x2f, 0xf0,

    /* U+590D "复" */
    0x10, 0x0, 0xff, 0xe6, 0x0, 0x17, 0xfc, 0x1f,
    0xf0, 0x40, 0x40, 0xfe, 0x7, 0xfc, 0x38, 0x20,
    0x1b, 0x0, 0xfe, 0x1c, 0x6,

    /* U+5B8C "完" */
    0x4, 0x0, 0x60, 0xff, 0xf8, 0x1, 0xbf, 0xd0,
    0x0, 0x0, 0xf, 0xff, 0x19, 0x1, 0x10, 0x11,
    0x13, 0x11, 0xe1, 0xf0,

    /* U+5B9A "定" */
    0x2, 0x0, 0x8, 0x7, 0xff, 0x90, 0x2, 0x40,
    0x8, 0x7f, 0x80, 0x20, 0x4, 0x80, 0x13, 0xe0,
    0xc8, 0x3, 0xa0, 0x1b, 0x80, 0xc3, 0xf8,

    /* U+5DF2 "已" */
    0xff, 0xc0, 0x4, 0x0, 0x44, 0x4, 0x40, 0x47,
    0xfc, 0x40, 0x4, 0x0, 0x40, 0x14, 0x1, 0x40,
    0x23, 0xfe,

    /* U+5E38 "常" */
    0x44, 0x22, 0x44, 0xff, 0xf8, 0x1, 0xbf, 0xd2,
    0x4, 0x3f, 0xc0, 0x40, 0x7f, 0xe4, 0x42, 0x44,
    0x24, 0x42, 0x44, 0xe0,

    /* U+5E72 "干" */
    0x7f, 0xf0, 0x10, 0x0, 0x80, 0x4, 0x0, 0x20,
    0x7f, 0xfc, 0x8, 0x0, 0x40, 0x2, 0x0, 0x10,
    0x0, 0x80, 0x4, 0x0,

    /* U+5EA6 "度" */
    0x3, 0x3, 0xff, 0x91, 0x10, 0xff, 0xe4, 0x44,
    0x23, 0xe1, 0x0, 0xf, 0xfc, 0x4c, 0x62, 0x36,
    0x21, 0xe1, 0x70, 0xe0,

    /* U+5F00 "开" */
    0x7f, 0xf8, 0xc2, 0x6, 0x10, 0x30, 0x81, 0x84,
    0x7f, 0xfc, 0x41, 0x2, 0x8, 0x10, 0x41, 0x2,
    0x18, 0x11, 0x80, 0x80,

    /* U+6062 "恢" */
    0x21, 0x1, 0x8, 0xd, 0xfe, 0xd2, 0x6, 0xa2,
    0x31, 0x54, 0x8b, 0x44, 0x6a, 0x24, 0x51, 0x23,
    0xa, 0x28, 0x43, 0x22, 0x30, 0x80,

    /* U+6210 "成" */
    0x1, 0x40, 0x9, 0x0, 0x48, 0xff, 0xe4, 0x10,
    0x20, 0x89, 0xf2, 0x88, 0x94, 0x44, 0xc2, 0x26,
    0x17, 0x75, 0x6, 0xa8, 0x63, 0x0,

    /* U+62A4 "护" */
    0x20, 0x82, 0x8, 0x20, 0x8f, 0xbf, 0x22, 0x12,
    0x21, 0x3b, 0xff, 0x21, 0xa2, 0x2, 0x20, 0x24,
    0x2, 0x40, 0xec, 0x0,

    /* U+63A5 "接" */
    0x21, 0x1, 0x4, 0x9, 0xfd, 0xe4, 0x42, 0x34,
    0x17, 0xfc, 0xc4, 0x1d, 0xff, 0x22, 0x21, 0x33,
    0x8, 0x70, 0x43, 0xcc, 0xe1, 0x0,

    /* U+6545 "故" */
    0x11, 0x0, 0x88, 0x4, 0x41, 0xfb, 0xe1, 0x32,
    0x9, 0x90, 0x5c, 0x8f, 0x24, 0x48, 0xc2, 0x46,
    0x12, 0x30, 0xf2, 0xc4, 0x63, 0x2, 0x0,

    /* U+6548 "效" */
    0x0, 0x2, 0x8, 0xfd, 0x0, 0x1f, 0x49, 0x24,
    0xf2, 0x8b, 0x27, 0x12, 0x30, 0xc3, 0xc, 0x28,
    0xc4, 0x96, 0x86, 0x30,

    /* U+65E0 "无" */
    0x7f, 0xf0, 0x10, 0x0, 0x80, 0x4, 0xf, 0xff,
    0x82, 0x80, 0x14, 0x1, 0xa0, 0x9, 0x0, 0x88,
    0x48, 0x43, 0x83, 0xf0,

    /* U+65E5 "日" */
    0xff, 0xc0, 0x60, 0x30, 0x18, 0xf, 0xfe, 0x3,
    0x1, 0x80, 0xc0, 0x7f, 0xf0, 0x10,

    /* U+65F6 "时" */
    0x0, 0x4f, 0x4, 0x90, 0x49, 0x7f, 0x90, 0x4f,
    0x64, 0x92, 0x49, 0x14, 0x91, 0x49, 0x4, 0xf0,
    0x40, 0x4, 0x1, 0xc0,

    /* U+672A "未" */
    0x4, 0x0, 0x40, 0x4, 0x7, 0xfe, 0x4, 0x0,
    0x40, 0xff, 0xf0, 0xe0, 0xd, 0x3, 0x48, 0x64,
    0x6c, 0x43, 0x4, 0x0,

    /* U+67E5 "查" */
    0x4, 0x0, 0x40, 0xff, 0xf0, 0xd0, 0x34, 0xcc,
    0x3, 0x3f, 0xc2, 0x4, 0x3f, 0xc2, 0x4, 0x3f,
    0xc0, 0x0, 0xff, 0xf0,

    /* U+68C0 "检" */
    0x20, 0x81, 0xc, 0x8, 0x51, 0xe4, 0x46, 0x41,
    0x3d, 0xf5, 0xc0, 0x16, 0x92, 0xa4, 0x91, 0x15,
    0x8, 0x8, 0x5f, 0xf2, 0x0, 0x0,

    /* U+6B62 "止" */
    0x1, 0x0, 0x8, 0x0, 0x40, 0x42, 0x2, 0x10,
    0x10, 0xfc, 0x84, 0x4, 0x20, 0x21, 0x1, 0x8,
    0x8, 0x41, 0xff, 0xf0,

    /* U+6B63 "正" */
    0x7f, 0xf0, 0x10, 0x0, 0x80, 0x44, 0x2, 0x20,
    0x11, 0xf0, 0x88, 0x4, 0x40, 0x22, 0x1, 0x10,
    0x3f, 0xfe,

    /* U+6C34 "水" */
    0x2, 0x0, 0x10, 0x0, 0x80, 0x4, 0x67, 0xa2,
    0x5, 0xa0, 0x6e, 0x2, 0x50, 0x32, 0x41, 0x13,
    0x10, 0x8e, 0x4, 0x0, 0xe0, 0x0,

    /* U+6CF5 "泵" */
    0x7f, 0xf0, 0x40, 0xf, 0xf9, 0xa0, 0x41, 0xfe,
    0x1, 0x0, 0x8, 0xf, 0x64, 0xb, 0xc0, 0x96,
    0x18, 0x8e, 0xc, 0x0,

    /* U+6D47 "浇" */
    0x43, 0x3, 0x8, 0xc3, 0xf8, 0x2, 0x20, 0xe,
    0x20, 0xe4, 0xb9, 0xe0, 0x0, 0x2f, 0xf9, 0x12,
    0x18, 0x90, 0x8c, 0x94, 0xc7, 0x80,

    /* U+6E05 "清" */
    0x1, 0x6, 0xff, 0x21, 0x0, 0xfe, 0x81, 0x5,
    0xef, 0xf, 0xe0, 0x82, 0x2f, 0xe4, 0x82, 0x4f,
    0xec, 0x82, 0x88, 0xe0,

    /* U+6E7F "湿" */
    0x6f, 0xf0, 0x40, 0x83, 0xfd, 0x90, 0x26, 0xff,
    0x0, 0x0, 0x9, 0x5, 0x4a, 0x26, 0x52, 0x32,
    0x90, 0x91, 0x3f, 0xf0,

    /* U+6E89 "溉" */
    0x0, 0x1, 0xbd, 0xf0, 0x91, 0x3, 0xd4, 0x49,
    0x51, 0xa5, 0x42, 0xff, 0xc2, 0xc, 0x2a, 0x30,
    0xac, 0xc2, 0xd5, 0x50, 0x25, 0x41, 0x9c,

    /* U+704C "灌" */
    0x42, 0x21, 0xff, 0xc0, 0x0, 0x1e, 0xe0, 0x95,
    0x37, 0xb8, 0xa, 0x0, 0xff, 0x1c, 0x41, 0x3f,
    0x89, 0xfc, 0x88, 0x84, 0x7f, 0x80,

    /* U+76C6 "盆" */
    0x8, 0x80, 0x82, 0x8, 0x9, 0xbf, 0xb0, 0x44,
    0x4, 0x20, 0xeb, 0xc, 0x20, 0x3f, 0xf1, 0x29,
    0x89, 0x4c, 0x4a, 0x6f, 0xff, 0x80,

    /* U+77E5 "知" */
    0x20, 0x4, 0x1f, 0x7f, 0x15, 0x11, 0x91, 0x11,
    0x11, 0xff, 0x11, 0x11, 0x11, 0x13, 0x91, 0x25,
    0x16, 0x3f, 0xc1, 0x10,

    /* U+7A7A "空" */
    0xc, 0x0, 0x40, 0xff, 0xf8, 0x1, 0x99, 0x92,
    0x6, 0xc0, 0x17, 0xfe, 0x4, 0x0, 0x40, 0x4,
    0x0, 0x40, 0xff, 0xf0,

    /* U+7EDC "络" */
    0x1, 0x1, 0x88, 0x8, 0xfc, 0xc4, 0x24, 0xd2,
    0x78, 0x60, 0x87, 0x8c, 0xc3, 0xf3, 0xf0, 0x10,
    0x86, 0x85, 0xc7, 0xe0, 0x21, 0x0,

    /* U+7F51 "网" */
    0xff, 0xf8, 0x1, 0x88, 0x5a, 0xa9, 0xb9, 0x99,
    0x99, 0x98, 0x99, 0x55, 0xa7, 0x5e, 0x21, 0x80,
    0x18, 0x7,

    /* U+81EA "自" */
    0x8, 0x2, 0xf, 0xfe, 0x1, 0x80, 0x7f, 0xf8,
    0x6, 0x1, 0xff, 0xe0, 0x18, 0x7, 0xff, 0x80,
    0x40,

    /* U+8D85 "超" */
    0x10, 0x0, 0x9f, 0xbf, 0x24, 0x21, 0x21, 0x11,
    0x7f, 0x38, 0x4f, 0xcb, 0xc2, 0x52, 0x16, 0x90,
    0xb4, 0xfd, 0x60, 0x9, 0xff, 0x80,

    /* U+8F6C "转" */
    0x20, 0xc2, 0x8, 0xfb, 0xf4, 0x8, 0x51, 0x85,
    0x7f, 0xf9, 0x1, 0x3f, 0x10, 0x2f, 0x84, 0x91,
    0xc1, 0xc, 0x10, 0x20,

    /* U+8FD4 "返" */
    0x0, 0x12, 0x3f, 0x9, 0x0, 0x8, 0x20, 0x7f,
    0x72, 0x10, 0x96, 0x84, 0x98, 0x29, 0xe1, 0xd8,
    0x98, 0x1, 0xa0, 0x8, 0xff, 0x80,

    /* U+8FDE "连" */
    0x41, 0x3, 0x7f, 0xcc, 0x80, 0x5, 0x0, 0x48,
    0x73, 0xf8, 0x82, 0x4, 0x10, 0x2f, 0xf9, 0x4,
    0x8, 0x20, 0xb0, 0x8, 0xff, 0x80,

    /* U+91CF "量" */
    0x3f, 0xe1, 0xff, 0x8, 0x8, 0x3f, 0x8f, 0xff,
    0x9f, 0xf8, 0xff, 0xc4, 0x46, 0x1f, 0xc3, 0xff,
    0x80, 0x81, 0xff, 0xf0,

    /* U+9501 "锁" */
    0x30, 0x40, 0x85, 0x27, 0xd5, 0x10, 0x10, 0x3d,
    0xf8, 0x45, 0x21, 0x14, 0x9f, 0x52, 0x11, 0x48,
    0x45, 0x21, 0x58, 0x86, 0x6c, 0x26, 0xc,

    /* U+95F2 "闲" */
    0x4f, 0xf2, 0x1, 0x20, 0x18, 0x21, 0x82, 0x1b,
    0xfd, 0x8b, 0x18, 0xa9, 0xb2, 0x5a, 0x21, 0x82,
    0x18, 0x7,

    /* U+9600 "阀" */
    0x0, 0xb, 0xfd, 0x80, 0x80, 0x19, 0x2b, 0x24,
    0x6f, 0xff, 0x93, 0x91, 0xb2, 0x26, 0x4d, 0xce,
    0x78, 0x6,

    /* U+9650 "限" */
    0xf7, 0xe9, 0x42, 0xa7, 0xea, 0x42, 0xa4, 0x29,
    0x7e, 0x95, 0x9, 0x4b, 0xb4, 0xc8, 0x44, 0x85,
    0x28, 0x61,

    /* U+9664 "除" */
    0x0, 0x87, 0x8c, 0x24, 0xd1, 0x48, 0x6a, 0xfe,
    0x50, 0x42, 0x42, 0x13, 0xfe, 0x90, 0x87, 0xa5,
    0x21, 0x25, 0x11, 0x28, 0x38, 0x0,

    /* U+969C "障" */
    0x0, 0x8f, 0x7f, 0x92, 0x4a, 0xff, 0xa0, 0xa,
    0x7e, 0x94, 0x29, 0x7e, 0x94, 0x2a, 0x3c, 0x8f,
    0xf8, 0x8, 0x80, 0x80,

    /* U+989D "额" */
    0x20, 0x2, 0x3f, 0xfc, 0x88, 0x48, 0x7b, 0xfc,
    0xa9, 0xf2, 0x97, 0xa9, 0xc6, 0x97, 0xa9, 0x48,
    0xc4, 0x92, 0x7a, 0x10,

    /* U+FF0C "，" */
    0xf6, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 50, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 72, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 106, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 6, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 124, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 24, .adv_w = 206, .box_w = 12, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 152, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 62, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 55, .adv_w = 76, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 61, .adv_w = 76, .box_w = 3, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 67, .adv_w = 105, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 71, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 78, .adv_w = 62, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 80, .adv_w = 78, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 81, .adv_w = 62, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 88, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 91, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 124, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 124, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 124, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 62, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 62, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 189, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 196, .adv_w = 124, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 199, .adv_w = 124, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 206, .adv_w = 106, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 212, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 235, .adv_w = 136, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 143, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 154, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 276, .adv_w = 132, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 154, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 163, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 66, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 317, .adv_w = 120, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 145, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 334, .adv_w = 122, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 182, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 162, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 166, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 166, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 404, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 134, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 134, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 162, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 129, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 197, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 128, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 483, .adv_w = 119, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 493, .adv_w = 135, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 76, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 509, .adv_w = 88, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 518, .adv_w = 76, .box_w = 3, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 524, .adv_w = 124, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 529, .adv_w = 125, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 530, .adv_w = 136, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 532, .adv_w = 126, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 538, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 547, .adv_w = 114, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 139, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 563, .adv_w = 124, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 73, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 126, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 586, .adv_w = 136, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 62, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 62, .box_w = 3, .box_h = 14, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 603, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 612, .adv_w = 64, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 615, .adv_w = 207, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 137, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 632, .adv_w = 136, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 139, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 648, .adv_w = 139, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 658, .adv_w = 87, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 662, .adv_w = 105, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 667, .adv_w = 84, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 672, .adv_w = 136, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 180, .box_w = 11, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 703, .adv_w = 117, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 713, .adv_w = 106, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 719, .adv_w = 76, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 725, .adv_w = 60, .box_w = 1, .box_h = 16, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 727, .adv_w = 76, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 733, .adv_w = 124, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 735, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 773, .adv_w = 224, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 791, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 814, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 836, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 858, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 880, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 900, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 920, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 938, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 958, .adv_w = 224, .box_w = 14, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 979, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 999, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1022, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1040, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1060, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1080, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1100, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1120, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1142, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1164, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1184, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1206, .adv_w = 224, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1229, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1249, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1269, .adv_w = 224, .box_w = 9, .box_h = 12, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 1283, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1303, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1323, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1343, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1365, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1385, .adv_w = 224, .box_w = 13, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1403, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1425, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1445, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1467, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1487, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1507, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1530, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1552, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1574, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1594, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1614, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1636, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1654, .adv_w = 224, .box_w = 10, .box_h = 13, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 1671, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1693, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1713, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1735, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1757, .adv_w = 224, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1777, .adv_w = 224, .box_w = 14, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1800, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1818, .adv_w = 224, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1836, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1854, .adv_w = 224, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1876, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1896, .adv_w = 224, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1916, .adv_w = 224, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x1, 0x23, 0xc0, 0x1d3, 0x245, 0x252, 0x369,
    0x49e, 0x8d4, 0x91e, 0xb03, 0xd82, 0xd90, 0xfe8, 0x102e,
    0x1068, 0x109c, 0x10f6, 0x1258, 0x1406, 0x149a, 0x159b, 0x173b,
    0x173e, 0x17d6, 0x17db, 0x17ec, 0x1920, 0x19db, 0x1ab6, 0x1d58,
    0x1d59, 0x1e2a, 0x1eeb, 0x1f3d, 0x1ffb, 0x2075, 0x207f, 0x2242,
    0x28bc, 0x29db, 0x2c70, 0x30d2, 0x3147, 0x33e0, 0x3f7b, 0x4162,
    0x41ca, 0x41d4, 0x43c5, 0x46f7, 0x47e8, 0x47f6, 0x4846, 0x485a,
    0x4892, 0x4a93, 0xb102
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 19978, .range_length = 45315, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 59, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 2, 0, 0, 0, 3, 4, 3,
    5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 6, 6, 0, 0, 0,
    0, 0, 7, 8, 9, 10, 11, 12,
    13, 0, 0, 14, 15, 16, 0, 0,
    10, 17, 10, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 2, 27, 0, 0,
    0, 0, 28, 29, 30, 0, 31, 32,
    33, 34, 0, 0, 35, 36, 34, 34,
    29, 29, 37, 38, 39, 40, 37, 41,
    42, 43, 44, 45, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 0, 0, 0, 0,
    2, 0, 3, 4, 0, 5, 6, 7,
    8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 9, 10, 0, 0, 0,
    11, 0, 12, 0, 13, 0, 0, 0,
    13, 0, 0, 14, 0, 0, 0, 0,
    13, 0, 13, 0, 15, 16, 17, 18,
    19, 20, 21, 22, 0, 23, 3, 0,
    0, 0, 24, 0, 25, 25, 25, 26,
    27, 0, 28, 29, 0, 0, 30, 30,
    25, 30, 25, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 0, 0, 3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, -29, 0, -29, 0,
    0, 0, 0, -14, 0, -24, -2, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    -7, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 20, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -24, 0, -34,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -25, -5, -16, -9, 0,
    -23, 0, 0, 0, -3, 0, 0, 0,
    6, 0, 0, -11, 0, -9, -6, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -4, -12, 0, -5, -2, -7, -17, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -6, 0, -2, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -10, -2, -20, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    -8, 0, -2, 6, 6, 0, 0, 2,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, -12, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -14, 0, -24,
    0, 0, 0, 0, 0, 0, -6, -2,
    -2, 0, 0, -14, -4, -3, 0, 1,
    -3, -2, -10, 6, 0, -2, 0, 0,
    0, 0, 6, -3, -2, -2, -1, -1,
    -2, 0, 0, 0, 0, -8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -3, -6, 0, -1, -1, -1, -3, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -3, -2, -2, -3, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, -7, -2, -6, -4,
    -3, -1, -1, -1, -2, -2, 0, 0,
    0, 0, -5, 0, 0, 0, 0, -7,
    -2, -3, -2, 0, -3, 0, 0, 0,
    0, -9, 0, 0, 0, -4, 0, 0,
    0, -2, 0, -10, 0, -6, 0, -2,
    -2, -4, -5, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, -6, 0, -2, 0, -8,
    -2, 0, 0, 0, 0, 0, -18, 0,
    -18, -17, 0, 0, 0, -9, -2, -35,
    -5, 0, 0, 1, 1, -6, 0, -8,
    0, -9, -3, 0, -6, 0, 0, -5,
    -5, -2, -4, -5, -4, -6, -4, -7,
    0, 0, 0, -7, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, -5,
    0, -3, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, -6, 0, 0, 0,
    0, 0, 0, -10, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -10,
    0, -7, 0, 0, 0, 0, -2, -2,
    -5, 0, -2, -4, -3, -3, -2, 0,
    -4, 0, 0, 0, -2, 0, 0, 0,
    -2, 0, 0, -8, -4, -5, -4, -4,
    -5, -3, 0, -22, 0, -38, 0, -13,
    0, 0, 0, 0, -8, 1, -6, 0,
    -6, -30, -7, -19, -14, 0, -19, 0,
    -20, 0, -3, -3, -1, 0, 0, 0,
    0, -5, -2, -9, -9, 0, -9, 0,
    0, 0, 0, 0, -28, -8, -28, -19,
    0, 0, 0, -13, 0, -37, -2, -6,
    0, 0, 0, -6, -2, -19, 0, -11,
    -6, 0, -8, 0, 0, 0, -2, 0,
    0, 0, 0, -3, 0, -5, 0, 0,
    0, -2, 0, -8, 0, 0, 0, 0,
    0, -1, 0, -4, -3, -3, 0, 1,
    2, -1, 0, -2, 0, -1, -2, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, 0, 0, -4,
    0, 3, 0, 0, 0, 0, 0, 0,
    0, -3, -3, -5, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -27, -18,
    -27, -22, -5, -5, 0, -10, -6, -32,
    -10, 0, 0, 0, 0, -5, -3, -13,
    0, -18, -17, -4, -18, 0, 0, -12,
    -15, -4, -12, -8, -9, -10, -8, -19,
    0, 0, 0, 0, -4, 0, -4, -8,
    0, 0, 0, -4, 0, -12, -2, 0,
    0, -1, 0, -2, -3, 0, 0, -1,
    0, 0, -2, 0, 0, 0, -1, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -16, -5, -16, -12, 0, 0,
    0, -3, -2, -18, -2, 0, -2, 2,
    0, 0, 0, -5, 0, -6, -4, 0,
    -5, 0, 0, -5, -3, 0, -8, -2,
    -2, -4, -2, -6, 0, 0, 0, 0,
    -9, -2, -9, -7, 0, 0, 0, 0,
    -2, -16, -2, 0, 0, 0, 0, 0,
    0, -2, 0, -4, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, -2, 0, -7,
    0, 0, 0, 0, 0, 1, -4, 0,
    -3, -5, -2, 0, 0, 0, 0, 0,
    0, -2, -2, -4, 0, 0, 0, 0,
    0, -4, -2, -4, -3, -2, -4, -3,
    0, 0, 0, 0, -23, -17, -23, -17,
    -6, -6, -2, -3, -3, -25, -4, -3,
    -2, 0, 0, 0, 0, -6, 0, -17,
    -10, 0, -15, 0, 0, -10, -10, -7,
    -9, -3, -6, -9, -3, -12, 0, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    0, -2, -5, -8, -8, 0, -2, -2,
    -2, 0, -3, -4, 0, -4, -5, -5,
    -4, 0, 0, 0, 0, -3, -6, -4,
    -4, -6, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -22, -7, -13, -7, 0,
    -18, 0, 0, 0, 0, 0, 8, 0,
    18, 0, 0, 0, 0, -5, -2, 0,
    3, 0, 0, 0, 0, -13, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, -6, 0, -4, -1, 0, -6, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, -7, 0, -6, -2, 2, -2, 0,
    0, 0, -3, 0, 0, 0, 0, -14,
    0, -5, 0, -1, -12, 0, -6, -4,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, -1, -1, -4, -1, -1, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -3,
    0, 0, -6, 0, 0, -2, -5, 0,
    -2, 0, 0, 0, 0, -2, 0, 2,
    2, 2, 2, 0, 0, 0, 0, -9,
    0, 2, 0, 0, 0, 0, -2, 0,
    0, -5, -5, -6, 0, -4, -2, 0,
    -6, 0, -5, -4, 0, 0, -2, 0,
    0, 0, 0, -2, 0, 1, 1, -2,
    1, 0, 4, 10, 12, 0, -13, -3,
    -13, -4, 0, 0, 6, 0, 0, 0,
    0, 12, 0, 17, 12, 8, 15, 0,
    16, -5, -2, 0, -4, 0, -2, 0,
    -1, 0, 0, 3, 0, -1, 0, -3,
    0, 0, 4, -8, 0, 0, 0, 12,
    0, 0, -9, 0, 0, 0, 0, -6,
    0, 0, 0, 0, -3, 0, 0, -4,
    -3, 0, 0, 0, 9, 0, 0, 0,
    0, -1, -1, 0, 4, -3, 0, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, -6, 0, -2,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    3, -11, 3, 0, 3, 3, -3, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    -3, 0, 0, -2, -4, 0, -2, 0,
    -2, 0, 0, -5, -3, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, -3, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -14, -6, -14, -9, 6, 6,
    0, -3, 0, -14, 0, 0, 0, 0,
    0, 0, 0, -2, 3, -6, -2, 0,
    -2, 0, 0, 0, -1, 0, 0, 6,
    5, 0, 6, -1, 0, 0, 0, -12,
    0, 2, 0, 0, 0, 0, -3, 0,
    0, 0, 0, -6, 0, -2, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 2, -6,
    2, 2, 4, 4, -6, 0, 0, 0,
    0, -3, 0, 0, 0, 0, -1, 0,
    0, -5, -3, 0, -2, 0, 0, 0,
    -2, -5, 0, 0, 0, -4, 0, 0,
    0, 0, 0, -3, -9, -2, -9, -5,
    0, 0, 0, -3, 0, -10, 0, -5,
    0, -2, 0, 0, -3, -2, 0, -5,
    -1, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, -3, -10, 0, -10, -2, 0, 0,
    0, -1, 0, -8, 0, -6, 0, -2,
    0, -3, -6, 0, 0, -2, -1, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, -4, -3, 0, 0, -6,
    2, -3, -2, 0, 0, 2, 0, 0,
    -2, 0, -1, -8, 0, -4, 0, -2,
    -9, 0, 0, -2, -4, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -9, 0,
    -9, -4, 0, 0, 0, 0, 0, -10,
    0, -5, 0, -1, 0, -1, -2, 0,
    0, -5, -1, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, -3, 0, -6,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    0, 0, 0, 0, -7, 0, 0, -6,
    -2, 0, -2, 0, 0, 0, 0, 0,
    -2, -1, 0, 0, -1, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 45,
    .right_class_cnt     = 38,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_cn_14 = {
#else
lv_font_t lv_font_cn_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_CN_14*/
#endif /*BOARD_HAS_LVGL*/

