#include <stdio.h>
#include "myi2c.h"
#include "gxhtc3.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";
extern float temp,humi;
extern uint8_t temp_int,humi_int;

static void gxhtc3_task(void *args)
{
    esp_err_t ret;

    while(1)
    {
        ret = gxhtc3_get_tah();
        if (ret!=ESP_OK) {
            ESP_LOGE(TAG,"GXHTC3 READ TAH ERROR.");
        }
        else{
            ESP_LOGI(TAG, "TEMP:%.1f HUMI:%.1f", temp, humi);
            ESP_LOGI(TAG, "TEMP:%d HUMI:%d", temp_int, humi_int);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    esp_err_t ret = gxhtc3_read_id();
    while(ret != ESP_OK)
    {
         ret = gxhtc3_read_id();
         ESP_LOGI(TAG,"GXHTC3 READ ID");
         vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    xTaskCreate(gxhtc3_task, "gxhtc3_task", 4096, NULL, 6, NULL);
}