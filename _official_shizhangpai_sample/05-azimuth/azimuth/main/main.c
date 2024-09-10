#include <stdio.h>
#include "myi2c.h"
#include "qmc5883l.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

t_sQMC5883L QMC5883L;

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    qmc5883l_init();

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        qmc5883l_fetch_azimuth(&QMC5883L);
        ESP_LOGI(TAG, "azimuth = %.1f", QMC5883L.azimuth);
    }
}

