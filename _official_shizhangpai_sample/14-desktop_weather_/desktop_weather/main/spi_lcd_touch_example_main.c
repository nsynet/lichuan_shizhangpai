/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "myi2c.h"
#include "lv_demos.h"
#include "esp_lcd_touch_ft5x06.h"
#include "driver/ledc.h"
#include "gxhtc3.h"

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "zlib.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>
#include "examples/libs/lv_example_libs.h"

#include <math.h>

static const char *TAG = "example";

// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           3
#define EXAMPLE_PIN_NUM_MOSI           5
#define EXAMPLE_PIN_NUM_MISO           -1
#define EXAMPLE_PIN_NUM_LCD_DC         6
#define EXAMPLE_PIN_NUM_LCD_RST        -1
#define EXAMPLE_PIN_NUM_LCD_CS         4
#define EXAMPLE_PIN_NUM_BK_LIGHT       2
#define EXAMPLE_PIN_NUM_TOUCH_CS       -1

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              240

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

esp_lcd_touch_handle_t tp = NULL;

extern void lv_gui_start(void); 
extern void lv_main_page(void);
extern void lv_qweather_icon_show(void);
extern void lv_week_show(void);
extern void lv_qair_level_show(void);

extern lv_obj_t * label_wifi;
extern lv_obj_t * label_sntp;
extern lv_obj_t * label_weather;
extern lv_obj_t * qweather_icon_label;
extern lv_obj_t * qweather_temp_label;
extern lv_obj_t * qweather_text_label;
extern lv_obj_t * qair_level_obj;
extern lv_obj_t * qair_level_label;
extern lv_obj_t * led_time_label;
extern lv_obj_t * week_label;
extern lv_obj_t * sunset_label;
extern lv_obj_t *indoor_temp_label;
extern lv_obj_t *indoor_humi_label;
extern lv_obj_t *outdoor_temp_label;
extern lv_obj_t *outdoor_humi_label;
extern lv_obj_t * date_label;

extern float temp, humi;

int reset_flag;
int th_update_flag;
int qwnow_update_flag;
int qair_update_flag;
int qwdaily_update_flag;

static EventGroupHandle_t my_event_group;

#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_GET_SNTP_BIT           BIT1
#define WIFI_GET_DAILYWEATHER_BIT   BIT2
#define WIFI_GET_RTWEATHER_BIT      BIT3
#define WIFI_GET_WEATHER_BIT        BIT4

#define MAX_HTTP_OUTPUT_BUFFER      2048

#define QWEATHER_DAILY_URL   "https://devapi.qweather.com/v7/weather/3d?&location=xxx&key=xxx"
#define QWEATHER_NOW_URL     "https://devapi.qweather.com/v7/weather/now?&location=xxx&key=xxx"
#define QAIR_NOW_URL         "https://devapi.qweather.com/v7/air/now?&location=xxx&key=xxx"

time_t now;
struct tm timeinfo;

int temp_value, humi_value; // 室内实时温湿度值

int qwnow_temp; // 实时天气温度
int qwnow_humi; // 实时天气湿度
int qwnow_icon; // 实时天气图标
char qwnow_text[32]; // 实时天气状态

int qwdaily_tempMax;       // 当天最高温度
int qwdaily_tempMin;       // 当天最低温度
char qwdaily_sunrise[10];  // 当天日出时间
char qwdaily_sunset[10];   // 当天日落时间

int qanow_level;       // 实时空气质量等级


static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

// LCD背光调节初始化函数
static void lcd_brightness_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT, // Set duty resolution to 13 bits,
        .freq_hz          = 5000, // Frequency in Hertz. Set frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = EXAMPLE_PIN_NUM_BK_LIGHT,
        .duty           = 0, // Set duty
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

// GZIP解压函数
int gzDecompress(char *src, int srcLen, char *dst, int* dstLen)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = srcLen;
    strm.avail_out = *dstLen;
    strm.next_in = (Bytef *)src;
    strm.next_out = (Bytef *)dst;

    int err = -1;
    err = inflateInit2(&strm, 31); // 初始化
    if (err == Z_OK)
    {
        printf("inflateInit2 err=Z_OK\n");
        err = inflate(&strm, Z_FINISH); // 解压gzip数据
        if (err == Z_STREAM_END) // 解压成功
        { 
            printf("inflate err=Z_OK\n");
            *dstLen = strm.total_out; 
        } 
        else // 解压失败
        {
            printf("inflate err=!Z_OK\n");
        }
        inflateEnd(&strm);
    } 
    else
    {
        printf("inflateInit2 err! err=%d\n", err);
    }

    return err;
}

// 获取每日天气预报
void get_daily_weather(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    int client_code = 0;
    int64_t gzip_len = 0;

    esp_http_client_config_t config = {
        .url = QWEATHER_DAILY_URL,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        client_code = esp_http_client_get_status_code(client);
        gzip_len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRIu64, client_code, gzip_len);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }

    if (client_code == 200)
    {
        int buffSize = MAX_HTTP_OUTPUT_BUFFER;
        char* buffData = (char*)malloc(MAX_HTTP_OUTPUT_BUFFER);
        memset(buffData, 0, MAX_HTTP_OUTPUT_BUFFER);

        int ret = gzDecompress(local_response_buffer, gzip_len, buffData, &buffSize);

        if (Z_STREAM_END == ret) { /* 解压成功 */
            printf("daily_weather decompress success\n");
            printf("buffSize = %d\n", buffSize);
            // lv_label_set_text(label_weather, "√ 天气信息获取成功");

            cJSON *root = cJSON_Parse(buffData);
            cJSON *daily = cJSON_GetObjectItem(root,"daily");

            cJSON *daily1 = cJSON_GetArrayItem(daily, 0);

            char *temp_max = cJSON_GetObjectItem(daily1,"tempMax")->valuestring;
            char *temp_min = cJSON_GetObjectItem(daily1,"tempMin")->valuestring;
            char *sunset = cJSON_GetObjectItem(daily1,"sunset")->valuestring;
            char *sunrise = cJSON_GetObjectItem(daily1,"sunrise")->valuestring;

            qwdaily_tempMax = atoi(temp_max);
            qwdaily_tempMin = atoi(temp_min);
            strcpy(qwdaily_sunrise, sunrise);
            strcpy(qwdaily_sunset, sunset);

            ESP_LOGI(TAG, "最高气温：%d", qwdaily_tempMax);
            ESP_LOGI(TAG, "最低气温：%d", qwdaily_tempMin);
            ESP_LOGI(TAG, "日出时间：%s", qwdaily_sunrise);
            ESP_LOGI(TAG, "日落时间：%s", qwdaily_sunset);

            cJSON_Delete(root);

            qwdaily_update_flag = 1;
        }
        else {
            printf("decompress failed:%d\n", ret);
        }
        free(buffData);
    }
    esp_http_client_cleanup(client);

}

// 获取实时天气信息
void get_now_weather(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    int client_code = 0;
    int64_t gzip_len = 0;

    esp_http_client_config_t config = {
        .url = QWEATHER_NOW_URL,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        client_code = esp_http_client_get_status_code(client);
        gzip_len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRIu64, client_code, gzip_len);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }

    if(client_code == 200)
    {
        int buffSize = MAX_HTTP_OUTPUT_BUFFER;
        char* buffData = (char*)malloc(MAX_HTTP_OUTPUT_BUFFER);
        memset(buffData, 0, MAX_HTTP_OUTPUT_BUFFER);

        int ret = gzDecompress(local_response_buffer, gzip_len, buffData, &buffSize);

        if (Z_STREAM_END == ret) { /* 解压成功 */
            printf("now weather decompress success\n");
            printf("buffSize = %d\n", buffSize);

            cJSON *root = cJSON_Parse(buffData);
            cJSON *now = cJSON_GetObjectItem(root,"now");

            char *temp = cJSON_GetObjectItem(now,"temp")->valuestring;
            char *icon = cJSON_GetObjectItem(now,"icon")->valuestring;
            char *humidity = cJSON_GetObjectItem(now,"humidity")->valuestring;

            qwnow_temp = atoi(temp);
            qwnow_humi = atoi(humidity);
            qwnow_icon = atoi(icon);

            ESP_LOGI(TAG, "地区：小店区");
            ESP_LOGI(TAG, "温度：%d", qwnow_temp);
            ESP_LOGI(TAG, "湿度：%d", qwnow_humi);
            ESP_LOGI(TAG, "图标：%d", qwnow_icon);

            cJSON_Delete(root);

            qwnow_update_flag = 1;
        }
        else {
            printf("decompress failed:%d\n", ret);
        }
        free(buffData);
    }
    esp_http_client_cleanup(client);
}

// 获取实时空气质量
void get_air_quality(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    int client_code = 0;
    int64_t gzip_len = 0;

    esp_http_client_config_t config = {
        .url = QAIR_NOW_URL,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        client_code = esp_http_client_get_status_code(client);
        gzip_len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRIu64, client_code, gzip_len);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }

    if(client_code == 200)
    {
        int buffSize = MAX_HTTP_OUTPUT_BUFFER;
        char* buffData = (char*)malloc(MAX_HTTP_OUTPUT_BUFFER);
        memset(buffData, 0, MAX_HTTP_OUTPUT_BUFFER);

        int ret = gzDecompress(local_response_buffer, gzip_len, buffData, &buffSize);

        if (Z_STREAM_END == ret) { /* 解压成功 */
            printf("decompress success\n");
            printf("buffSize = %d\n", buffSize);

            cJSON *root = cJSON_Parse(buffData);
            cJSON *now = cJSON_GetObjectItem(root,"now");

            char *level = cJSON_GetObjectItem(now,"level")->valuestring;

            qanow_level = atoi(level);

            ESP_LOGI(TAG, "空气质量：%d", qanow_level);

            cJSON_Delete(root);

            qair_update_flag = 1;
        }
        else {
            printf("decompress failed:%d\n", ret);
        }
        free(buffData);
    }
    esp_http_client_cleanup(client);
}

// 获取温湿度的任务函数
static void get_th_task(void *args)
{
    esp_err_t ret;
    int time_cnt = 0, date_cnt = 0;
    float temp_sum = 0.0, humi_sum = 0.0;

    while(1)
    {
        ret = gxhtc3_get_tah(); // 获取一次温湿度
        if (ret!=ESP_OK) {
            ESP_LOGE(TAG,"GXHTC3 READ TAH ERROR."); 
        }
        else{ // 如果成功获取数据
            temp_sum = temp_sum + temp; // 温度累计和
            humi_sum = humi_sum + humi; // 湿度累计和
            date_cnt++; // 记录累计次数
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时1秒
        time_cnt++; // 每秒+1
        if(time_cnt>10) // 10秒钟到
        {
            // 取平均数 且把结果四舍五入为整数
            temp_value = round(temp_sum/date_cnt); 
            humi_value = round(humi_sum/date_cnt); 
            // 各标志位清零
            time_cnt = 0; date_cnt = 0; temp_sum = 0; humi_sum = 0;
            // 标记温湿度有新数值
            th_update_flag = 1; 
            ESP_LOGI(TAG, "TEMP:%d HUMI:%d", temp_value, humi_value);
        }
    }

    vTaskDelete(NULL);
}

// WIFI连接 任务函数
static void wifi_connect_task(void *pvParameters)
{
    my_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Successfully Connected to AP");

    if(reset_flag == 1) // 如果是刚开机
    {
        lv_label_set_text(label_wifi, "√ WiFi连接成功");
        lv_label_set_text(label_sntp, "正在获取网络时间");
    }

    xEventGroupSetBits(my_event_group, WIFI_CONNECTED_BIT);

    vTaskDelete(NULL);
}

// 获得日期时间 任务函数
static void get_time_task(void *pvParameters)
{
    xEventGroupWaitBits(my_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    // wait for time to be set
    int retry = 0;
    const int retry_count = 6;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    if(retry>5)
    {
        esp_restart(); // 没有获取到时间的话 重启ESP32
    }

    esp_netif_sntp_deinit();
    // 设置时区
    setenv("TZ", "CST-8", 1); 
    tzset();
    // 获取系统时间
    time(&now);
    localtime_r(&now, &timeinfo);

    if(reset_flag == 1) // 如果是刚开机
    {
        lv_label_set_text(label_sntp, "√ 网络时间获取成功");
        lv_label_set_text(label_weather, "正在获取天气信息");
    }

    xEventGroupSetBits(my_event_group, WIFI_GET_SNTP_BIT);
    
    vTaskDelete(NULL);
}

// 获取每日天气预报的任务函数
static void get_dwather_task(void *pvParameters)
{
    xEventGroupWaitBits(my_event_group, WIFI_GET_SNTP_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    //vTaskDelay(pdMS_TO_TICKS(100));

    get_daily_weather();

    if (qwdaily_update_flag == 1)
    {
        qwdaily_update_flag = 0;
        xEventGroupSetBits(my_event_group, WIFI_GET_DAILYWEATHER_BIT);
    }
    else
    {
        printf("获取3日天气信息失败\n");
    }
    
    vTaskDelete(NULL);
}

// 获取实时天气信息的任务函数
static void get_rtweather_task(void *pvParameters)
{

    xEventGroupWaitBits(my_event_group, WIFI_GET_DAILYWEATHER_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    //vTaskDelay(pdMS_TO_TICKS(100));
    get_now_weather();
    if (qwnow_update_flag == 1) // 获取实时天气信息成功
    {
        xEventGroupSetBits(my_event_group, WIFI_GET_RTWEATHER_BIT);
    }
    vTaskDelete(NULL);
}

// 获取实时空气质量的任务函数
static void get_airq_task(void *pvParameters)
{
    // 等待获取完实时空气质量 再获取空气质量
    xEventGroupWaitBits(my_event_group, WIFI_GET_RTWEATHER_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    //vTaskDelay(pdMS_TO_TICKS(100));
    get_air_quality();

    if (qair_update_flag == 1)
    {
        lv_label_set_text(label_weather, "√ 天气信息获取成功");
        xEventGroupSetBits(my_event_group, WIFI_GET_WEATHER_BIT);
    }

    vTaskDelete(NULL);
}

// 主界面各值更新函数
void value_update_cb(lv_timer_t * timer)
{
    // 更新日期 星期 时分秒
    time(&now);
    localtime_r(&now, &timeinfo);
    lv_label_set_text_fmt(led_time_label, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_label_set_text_fmt(date_label, "%d年%02d月%02d日", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday);
    lv_week_show();

    // 日出日落时间交替显示 每隔5秒切换
    if (timeinfo.tm_sec%10 == 0) 
        lv_label_set_text_fmt(sunset_label, "日落 %s", qwdaily_sunset);
    else if(timeinfo.tm_sec%10 == 5)
        lv_label_set_text_fmt(sunset_label, "日出 %s", qwdaily_sunrise);

    // 更新温湿度 
    if(th_update_flag == 1)
    {
        th_update_flag = 0;
        lv_label_set_text_fmt(indoor_temp_label, "%d℃", temp_value);
        lv_label_set_text_fmt(indoor_humi_label, "%d%%", humi_value);
    }
    // 更新实时天气
    if(qwnow_update_flag == 1)
    {
        qwnow_update_flag = 0;
        lv_qweather_icon_show(); // 更新天气图标
        lv_label_set_text_fmt(qweather_text_label, "%s", qwnow_text); // 更新天气情况文字描述
        lv_label_set_text_fmt(outdoor_temp_label, "%d℃", qwnow_temp); // 更新室外温度
        lv_label_set_text_fmt(outdoor_humi_label, "%d%%", qwnow_humi); // 更新室外湿度
    }
    // 更新空气质量
    if(qair_update_flag ==1)
    {
        qair_update_flag = 0;
        lv_qair_level_show();
    }
    // 更新每日天气
    if(qwdaily_update_flag == 1)
    {
        qwdaily_update_flag = 0;
        lv_label_set_text_fmt(qweather_temp_label, "%d~%d℃", qwdaily_tempMin, qwdaily_tempMax); // 温度范围
    }
}

// 主界面 任务函数
static void main_page_task(void *pvParameters)
{
    int tm_cnt1 = 0;
    int tm_cnt2 = 0;

    xEventGroupWaitBits(my_event_group, WIFI_GET_WEATHER_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    example_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_obj_clean(lv_scr_act());
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_main_page();

    th_update_flag = 0;
    qwnow_update_flag = 0;
    qair_update_flag = 0;
    qwdaily_update_flag = 0;

    lv_timer_create(value_update_cb, 1000, NULL);  // 创建一个lv_timer 每秒更新一次数据

    reset_flag = 0; // 标记开机完成

    while (1)
    {
        tm_cnt1++;
        if (tm_cnt1 > 1800) // 30分钟更新一次实时天气和实时空气质量
        {
            tm_cnt1 = 0; // 计数清零
            example_connect();  // 连接wifi
            get_now_weather();  // 获取实时天气信息
            get_air_quality();  // 获取实时空气质量
            tm_cnt2++;
            if (tm_cnt2 > 1) // 60分钟更新一次每日天气
            {
                tm_cnt2 = 0;
                get_daily_weather(); // 获取每日天气信息
            }
            example_disconnect(); // 断开wifi
            printf("weather update time:%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));    
    }
    
    vTaskDelete(NULL);
}

// 主函数
void app_main(void)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    //初始化I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // 检查温湿度芯片
    ret = gxhtc3_read_id();
    while(ret != ESP_OK)
    {
         ret = gxhtc3_read_id();
         ESP_LOGI(TAG,"GXHTC3 READ ID");
         vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // 初始化液晶屏
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 初始化触摸屏
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_V_RES,
        .y_max = EXAMPLE_LCD_H_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 1,                                           
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller FT6336");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

    // 初始化LVGL
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    static lv_indev_drv_t indev_drv;    // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;

    lv_indev_drv_register(&indev_drv);

    // 初始化液晶屏背光
    lcd_brightness_init();
    // 设置占空比
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191*(1-0.5)));  // 0.5表示占空比是50%
    // 更新背光
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    lv_gui_start(); // 显示开机界面

    reset_flag = 1; // 标记刚开机

    xTaskCreate(get_th_task, "get_th_task", 4096, NULL, 5, NULL);               // 非一次性任务 获取室内温湿度
    xTaskCreate(wifi_connect_task, "wifi_connect_task", 8192, NULL, 5, NULL);   // 一次性任务   连接WIFI
    xTaskCreate(get_time_task, "get_time_task", 8192, NULL, 5, NULL);           // 一次性任务   获取网络时间
    xTaskCreate(get_dwather_task, "get_dwather_task", 8192, NULL, 5, NULL);     // 一次性任务   获取每日天气信息
    xTaskCreate(get_rtweather_task, "get_rtweather_task", 8192, NULL, 5, NULL); // 一次性任务   获取实时天气信息
    xTaskCreate(get_airq_task, "get_airq_task", 8192, NULL, 5, NULL);           // 一次性任务   获取实时空气质量
    xTaskCreate(main_page_task, "main_page_task", 8192, NULL, 5, NULL);         // 非一次性任务 主界面任务

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
