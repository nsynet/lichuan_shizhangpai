/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/widgets/extra/meter.html#simple-meter

#include "lvgl.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "driver/ledc.h"

LV_IMG_DECLARE(image_taikong);
LV_FONT_DECLARE(font_alipuhui);
LV_FONT_DECLARE(font_qweather);
LV_FONT_DECLARE(font_led);
LV_FONT_DECLARE(font_myawesome);


lv_obj_t * label_wifi;
lv_obj_t * label_sntp;
lv_obj_t * label_weather;

lv_obj_t * qweather_icon_label;
lv_obj_t * qweather_temp_label;
lv_obj_t * qweather_text_label;
lv_obj_t * qair_level_obj;
lv_obj_t * qair_level_label;
lv_obj_t * led_time_label;
lv_obj_t * week_label;
lv_obj_t * sunset_label;
lv_obj_t *indoor_temp_label;
lv_obj_t *indoor_humi_label;
lv_obj_t *outdoor_temp_label;
lv_obj_t *outdoor_humi_label;
lv_obj_t * date_label;

extern time_t now;
extern struct tm timeinfo;

extern int temp_value, humi_value;

extern int qwnow_temp; // 实时天气温度
extern int qwnow_humi; // 实时天气湿度
extern int qwnow_icon; // 实时天气图标
extern char qwnow_text[32]; // 实时天气状态

extern int qwdaily_tempMax;       // 当天最高温度
extern int qwdaily_tempMin;       // 当天最低温度
extern char qwdaily_sunrise[10];  // 当天日出时间
extern char qwdaily_sunset[10];   // 当天日落时间

extern int qanow_level;       // 实时空气质量等级

// 开机界面
void lv_gui_start(void)
{
    // 设置背景色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x080808), 0);

    // 显示太空人GIF图片
    lv_obj_t *gif_start = lv_gif_create(lv_scr_act());
    lv_gif_set_src(gif_start, &image_taikong);
    lv_obj_align(gif_start, LV_ALIGN_TOP_MID, 0, 20);

    // 连接wifi
    label_wifi = lv_label_create(lv_scr_act());
    lv_label_set_text(label_wifi, "正在连接WiFi");
    lv_obj_set_style_text_font(label_wifi, &font_alipuhui, 0);
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_pos(label_wifi, 85 ,110);

    // 获取网络时间
    label_sntp = lv_label_create(lv_scr_act());
    lv_label_set_text(label_sntp, "");
    lv_obj_set_style_text_font(label_sntp, &font_alipuhui, 0);
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_pos(label_sntp, 70 ,135);

    // 获取天气信息
    label_weather = lv_label_create(lv_scr_act());
    lv_label_set_text(label_weather, "");
    lv_obj_set_style_text_font(label_weather, &font_alipuhui, 0);
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_pos(label_weather, 70 ,160);
}

// 显示天气图标
void lv_qweather_icon_show(void)
{
    switch (qwnow_icon)
    {
        case 100: lv_label_set_text(qweather_icon_label, "\xEF\x84\x81"); strcpy(qwnow_text, "晴"); break;
        case 101: lv_label_set_text(qweather_icon_label, "\xEF\x84\x82"); strcpy(qwnow_text, "多云"); break;
        case 102: lv_label_set_text(qweather_icon_label, "\xEF\x84\x83"); strcpy(qwnow_text, "少云"); break;
        case 103: lv_label_set_text(qweather_icon_label, "\xEF\x84\x84"); strcpy(qwnow_text, "晴间多云"); break;
        case 104: lv_label_set_text(qweather_icon_label, "\xEF\x84\x85"); strcpy(qwnow_text, "阴"); break;
        case 150: lv_label_set_text(qweather_icon_label, "\xEF\x84\x86"); strcpy(qwnow_text, "晴"); break;
        case 151: lv_label_set_text(qweather_icon_label, "\xEF\x84\x87"); strcpy(qwnow_text, "多云"); break;
        case 152: lv_label_set_text(qweather_icon_label, "\xEF\x84\x88"); strcpy(qwnow_text, "少云"); break;
        case 153: lv_label_set_text(qweather_icon_label, "\xEF\x84\x89"); strcpy(qwnow_text, "晴间多云"); break;
        case 300: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8A"); strcpy(qwnow_text, "阵雨"); break;
        case 301: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8B"); strcpy(qwnow_text, "强阵雨"); break;
        case 302: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8C"); strcpy(qwnow_text, "雷阵雨"); break;
        case 303: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8D"); strcpy(qwnow_text, "强雷阵雨"); break;
        case 304: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8E"); strcpy(qwnow_text, "雷阵雨伴有冰雹"); break;
        case 305: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8F"); strcpy(qwnow_text, "小雨"); break;
        case 306: lv_label_set_text(qweather_icon_label, "\xEF\x84\x90"); strcpy(qwnow_text, "中雨"); break;
        case 307: lv_label_set_text(qweather_icon_label, "\xEF\x84\x91"); strcpy(qwnow_text, "大雨"); break;
        case 308: lv_label_set_text(qweather_icon_label, "\xEF\x84\x92"); strcpy(qwnow_text, "极端降雨"); break;
        case 309: lv_label_set_text(qweather_icon_label, "\xEF\x84\x93"); strcpy(qwnow_text, "毛毛雨"); break;
        case 310: lv_label_set_text(qweather_icon_label, "\xEF\x84\x94"); strcpy(qwnow_text, "暴雨"); break;
        case 311: lv_label_set_text(qweather_icon_label, "\xEF\x84\x95"); strcpy(qwnow_text, "大暴雨"); break;
        case 312: lv_label_set_text(qweather_icon_label, "\xEF\x84\x96"); strcpy(qwnow_text, "特大暴雨"); break;
        case 313: lv_label_set_text(qweather_icon_label, "\xEF\x84\x97"); strcpy(qwnow_text, "冻雨"); break;
        case 314: lv_label_set_text(qweather_icon_label, "\xEF\x84\x98"); strcpy(qwnow_text, "小到中雨"); break;
        case 315: lv_label_set_text(qweather_icon_label, "\xEF\x84\x99"); strcpy(qwnow_text, "中到大雨"); break;
        case 316: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9A"); strcpy(qwnow_text, "大到暴雨"); break;
        case 317: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9B"); strcpy(qwnow_text, "暴雨到大暴雨"); break;
        case 318: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9C"); strcpy(qwnow_text, "大暴雨到特大暴雨"); break;
        case 350: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9D"); strcpy(qwnow_text, "阵雨"); break;
        case 351: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9E"); strcpy(qwnow_text, "强阵雨"); break;
        case 399: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9F"); strcpy(qwnow_text, "雨"); break;
        case 400: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA0"); strcpy(qwnow_text, "小雪"); break;
        case 401: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA1"); strcpy(qwnow_text, "中雪"); break;
        case 402: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA2"); strcpy(qwnow_text, "大雪"); break;
        case 403: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA3"); strcpy(qwnow_text, "暴雪"); break;
        case 404: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA4"); strcpy(qwnow_text, "雨夹雪"); break;
        case 405: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA5"); strcpy(qwnow_text, "雨雪天气"); break;
        case 406: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA6"); strcpy(qwnow_text, "阵雨夹雪"); break;
        case 407: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA7"); strcpy(qwnow_text, "阵雪"); break;
        case 408: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA8"); strcpy(qwnow_text, "小到中雪"); break;
        case 409: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA9"); strcpy(qwnow_text, "中到大雪"); break;
        case 410: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAA"); strcpy(qwnow_text, "大到暴雪"); break;
        case 456: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAB"); strcpy(qwnow_text, "阵雨夹雪"); break;
        case 457: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAC"); strcpy(qwnow_text, "阵雪"); break;
        case 499: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAD"); strcpy(qwnow_text, "雪"); break;
        case 500: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAE"); strcpy(qwnow_text, "薄雾"); break;
        case 501: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAF"); strcpy(qwnow_text, "雾"); break;
        case 502: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB0"); strcpy(qwnow_text, "霾"); break;
        case 503: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB1"); strcpy(qwnow_text, "扬沙"); break;
        case 504: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB2"); strcpy(qwnow_text, "浮尘"); break;
        case 507: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB3"); strcpy(qwnow_text, "沙尘暴"); break;
        case 508: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB4"); strcpy(qwnow_text, "强沙尘暴"); break;
        case 509: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB5"); strcpy(qwnow_text, "浓雾"); break;
        case 510: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB6"); strcpy(qwnow_text, "强浓雾"); break;
        case 511: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB7"); strcpy(qwnow_text, "中度霾"); break;
        case 512: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB8"); strcpy(qwnow_text, "重度霾"); break;
        case 513: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB9"); strcpy(qwnow_text, "严重霾"); break;
        case 514: lv_label_set_text(qweather_icon_label, "\xEF\x84\xBA"); strcpy(qwnow_text, "大雾"); break;
        case 515: lv_label_set_text(qweather_icon_label, "\xEF\x84\xBB"); strcpy(qwnow_text, "特强浓雾"); break;
        case 900: lv_label_set_text(qweather_icon_label, "\xEF\x85\x84"); strcpy(qwnow_text, "热"); break;
        case 901: lv_label_set_text(qweather_icon_label, "\xEF\x85\x85"); strcpy(qwnow_text, "冷"); break;
    
        default:
            printf("ICON_CODE:%d\n", qwnow_icon);
            lv_label_set_text(qweather_icon_label, "\xEF\x85\x86");
            strcpy(qwnow_text, "未知天气");
            break;
    }
}

// 显示星期几
void lv_week_show(void)
{
    switch (timeinfo.tm_wday)
    {
        case 0: lv_label_set_text(week_label, "星期日"); break;
        case 1: lv_label_set_text(week_label, "星期一"); break;
        case 2: lv_label_set_text(week_label, "星期二"); break;
        case 3: lv_label_set_text(week_label, "星期三"); break;
        case 4: lv_label_set_text(week_label, "星期四"); break; 
        case 5: lv_label_set_text(week_label, "星期五"); break;
        case 6: lv_label_set_text(week_label, "星期六"); break;
        default: lv_label_set_text(week_label, "星期日"); break;
    }
}

// 显示空气质量
void lv_qair_level_show(void)
{
    switch (qanow_level)
    {
        case 1: 
            lv_label_set_text(qair_level_label, "优"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_GREEN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        case 2: 
            lv_label_set_text(qair_level_label, "良"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_YELLOW), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0x000000), 0);
            break;
        case 3: 
            lv_label_set_text(qair_level_label, "轻");
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_ORANGE), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0); 
            break;
        case 4: 
            lv_label_set_text(qair_level_label, "中"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_RED), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break; 
        case 5: 
            lv_label_set_text(qair_level_label, "重"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_PURPLE), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        case 6: 
            lv_label_set_text(qair_level_label, "严"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_BROWN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        default: 
            lv_label_set_text(qair_level_label, "未"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_GREEN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
    }
}

// 主界面
void lv_main_page(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0); // 修改背景为黑色

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 10);  // 设置圆角半径
    lv_style_set_bg_opa( &style, LV_OPA_COVER );
    lv_style_set_bg_color(&style, lv_color_hex(0x00BFFF));
    lv_style_set_border_width(&style, 0);
    lv_style_set_pad_all(&style, 10);
    lv_style_set_width(&style, 320);  // 设置宽
    lv_style_set_height(&style, 240); // 设置高

    /*Create an object with the new style*/
    lv_obj_t * obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(obj, &style, 0);

    // 显示地理位置
    lv_obj_t * addr_label = lv_label_create(obj);
    lv_obj_set_style_text_font(addr_label, &font_alipuhui, 0);
    lv_label_set_text(addr_label, "太原市|小店区");
    lv_obj_align_to(addr_label, obj, LV_ALIGN_TOP_LEFT, 0, 0);

    // 显示年月日
    date_label = lv_label_create(obj);
    lv_obj_set_style_text_font(date_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(date_label, "%d年%02d月%02d日", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday);
    lv_obj_align_to(date_label, obj, LV_ALIGN_TOP_RIGHT, 0, 0);

    // 显示分割线
    lv_obj_t * above_bar = lv_bar_create(obj);
    lv_obj_set_size(above_bar, 300, 3);
    lv_obj_set_pos(above_bar, 0 , 30);
    lv_bar_set_value(above_bar, 100, LV_ANIM_OFF);

    // 显示天气图标
    qweather_icon_label = lv_label_create(obj);
    lv_obj_set_style_text_font(qweather_icon_label, &font_qweather, 0);
    lv_obj_set_pos(qweather_icon_label, 0 , 40);
    lv_qweather_icon_show();

    // 显示空气质量
    static lv_style_t qair_level_style;
    lv_style_init(&qair_level_style);
    lv_style_set_radius(&qair_level_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&qair_level_style, lv_palette_main(LV_PALETTE_GREEN)); // 绿色
    lv_style_set_text_color(&qair_level_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&qair_level_style, 0);
    lv_style_set_pad_all(&qair_level_style, 0);
    lv_style_set_width(&qair_level_style, 50);  // 设置宽
    lv_style_set_height(&qair_level_style, 26); // 设置高

    qair_level_obj = lv_obj_create(obj);
    lv_obj_add_style(qair_level_obj, &qair_level_style, 0);
    lv_obj_align_to(qair_level_obj, qweather_icon_label, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);

    qair_level_label = lv_label_create(qair_level_obj);
    lv_obj_set_style_text_font(qair_level_label, &font_alipuhui, 0);
    lv_obj_align(qair_level_label, LV_ALIGN_CENTER, 0, 0);
    lv_qair_level_show();

    // 显示当天室外温度范围
    qweather_temp_label = lv_label_create(obj);
    lv_obj_set_style_text_font(qweather_temp_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(qweather_temp_label, "%d~%d℃", qwdaily_tempMin, qwdaily_tempMax);
    lv_obj_align_to(qweather_temp_label, qweather_icon_label, LV_ALIGN_OUT_RIGHT_MID, 5, 5);

    // 显示当天天气图标代表的天气状况
    qweather_text_label = lv_label_create(obj);
    lv_obj_set_style_text_font(qweather_text_label, &font_alipuhui, 0);
    lv_label_set_long_mode(qweather_text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
    lv_obj_set_width(qweather_text_label, 80);
    lv_label_set_text_fmt(qweather_text_label, "%s", qwnow_text);
    lv_obj_align_to(qweather_text_label, qweather_icon_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);
    
    // 显示时间  小时:分钟:秒钟
    led_time_label = lv_label_create(obj);
    lv_obj_set_style_text_font(led_time_label, &font_led, 0);
    lv_label_set_text_fmt(led_time_label, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_obj_set_pos(led_time_label, 142, 42);

    // 显示星期几
    week_label = lv_label_create(obj);
    lv_obj_set_style_text_font(week_label, &font_alipuhui, 0);
    lv_obj_align_to(week_label, led_time_label, LV_ALIGN_OUT_BOTTOM_RIGHT, -10, 6);
    lv_week_show();

    // 显示日落时间 
    sunset_label = lv_label_create(obj);
    lv_obj_set_style_text_font(sunset_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(sunset_label, "日落 %s", qwdaily_sunset);
    lv_obj_set_pos(sunset_label, 200 , 103);

    // 显示分割线
    lv_obj_t * below_bar = lv_bar_create(obj);
    lv_obj_set_size(below_bar, 300, 3);
    lv_obj_set_pos(below_bar, 0, 130);
    lv_bar_set_value(below_bar, 100, LV_ANIM_OFF);

    // 显示室外温湿度
    static lv_style_t outdoor_style;
    lv_style_init(&outdoor_style);
    lv_style_set_radius(&outdoor_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&outdoor_style, lv_color_hex(0xd8b010)); // 
    lv_style_set_text_color(&outdoor_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&outdoor_style, 0);
    lv_style_set_pad_all(&outdoor_style, 5);
    lv_style_set_width(&outdoor_style, 100);  // 设置宽
    lv_style_set_height(&outdoor_style, 80); // 设置高

    lv_obj_t * outdoor_obj = lv_obj_create(obj);
    lv_obj_add_style(outdoor_obj, &outdoor_style, 0);
    lv_obj_align(outdoor_obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *outdoor_th_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_th_label, &font_alipuhui, 0);
    lv_label_set_text(outdoor_th_label, "室外");
    lv_obj_align(outdoor_th_label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *temp_symbol_label1 = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(temp_symbol_label1, &font_myawesome, 0);
    lv_label_set_text(temp_symbol_label1, "\xEF\x8B\x88");  // 显示温度图标
    lv_obj_align(temp_symbol_label1, LV_ALIGN_LEFT_MID, 10, 0);

    outdoor_temp_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_temp_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(outdoor_temp_label, "%d℃", qwnow_temp);
    lv_obj_align_to(outdoor_temp_label, temp_symbol_label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *humi_symbol_label1 = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(humi_symbol_label1, &font_myawesome, 0);
    lv_label_set_text(humi_symbol_label1, "\xEF\x81\x83");  // 显示湿度图标
    lv_obj_align(humi_symbol_label1, LV_ALIGN_BOTTOM_LEFT, 10, 0);

    outdoor_humi_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_humi_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(outdoor_humi_label, "%d%%", qwnow_humi);
    lv_obj_align_to(outdoor_humi_label, humi_symbol_label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 显示室内温湿度
    static lv_style_t indoor_style;
    lv_style_init(&indoor_style);
    lv_style_set_radius(&indoor_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&indoor_style, lv_color_hex(0xfe6464)); //
    lv_style_set_text_color(&indoor_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&indoor_style, 0);
    lv_style_set_pad_all(&indoor_style, 5);
    lv_style_set_width(&indoor_style, 100);  // 设置宽
    lv_style_set_height(&indoor_style, 80); // 设置高

    lv_obj_t * indoor_obj = lv_obj_create(obj);
    lv_obj_add_style(indoor_obj, &indoor_style, 0);
    lv_obj_align(indoor_obj, LV_ALIGN_BOTTOM_MID, 10, 0);

    lv_obj_t *indoor_th_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_th_label, &font_alipuhui, 0);
    lv_label_set_text(indoor_th_label, "室内");
    lv_obj_align(indoor_th_label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *temp_symbol_label2 = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(temp_symbol_label2, &font_myawesome, 0);
    lv_label_set_text(temp_symbol_label2, "\xEF\x8B\x88");  // 温度图标
    lv_obj_align(temp_symbol_label2, LV_ALIGN_LEFT_MID, 10, 0);

    indoor_temp_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_temp_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(indoor_temp_label, "%d℃", temp_value);
    lv_obj_align_to(indoor_temp_label, temp_symbol_label2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *humi_symbol_label2 = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(humi_symbol_label2, &font_myawesome, 0);
    lv_label_set_text(humi_symbol_label2, "\xEF\x81\x83");  // 湿度图标
    lv_obj_align(humi_symbol_label2, LV_ALIGN_BOTTOM_LEFT, 10, 0);

    indoor_humi_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_humi_label, &font_alipuhui, 0);
    lv_label_set_text_fmt(indoor_humi_label, "%d%%", humi_value);
    lv_obj_align_to(indoor_humi_label, humi_symbol_label2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 显示太空人
    lv_obj_t *tk_gif = lv_gif_create(obj);
    lv_gif_set_src(tk_gif, &image_taikong);
    lv_obj_align(tk_gif, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

}


