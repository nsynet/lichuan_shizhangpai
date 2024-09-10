#include <stdio.h>
#include "myi2c.h"
#include "qmi8658c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

t_sQMI8658C QMI8658C;

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    qmi8658c_init();
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        qmi8658c_fetch_angleFromAcc(&QMI8658C);
        ESP_LOGI(TAG, "angle_x = %.1f  angle_y = %.1f angle_y = %.1f",QMI8658C.AngleX, QMI8658C.AngleY, QMI8658C.AngleZ);
    }
}
