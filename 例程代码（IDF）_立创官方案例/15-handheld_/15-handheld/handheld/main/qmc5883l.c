#include "qmc5883l.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "myi2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "QMC5883L";

esp_err_t qmc5883L_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, QMC5883L_SENSOR_ADDR,  &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t qmc5883L_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    return i2c_master_write_to_device(I2C_MASTER_NUM, QMC5883L_SENSOR_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}


void qmc5883l_init(void)
{
    uint8_t id = 0;

    qmc5883L_register_read(QMC5883L_CHIPID, &id ,1);
    while (id != 0xff)  // 确定ID号是否正确
    {
        qmc5883L_register_read(QMC5883L_CHIPID, &id ,1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "QMC5883L OK!");
    qmc5883L_register_write_byte(QMC5883L_CTRL2, 0x80); // 复位芯片 
    vTaskDelay(10 / portTICK_PERIOD_MS);
    qmc5883L_register_write_byte(QMC5883L_CTRL1, 0x05); //Continuous模式 50Hz 
    qmc5883L_register_write_byte(QMC5883L_CTRL2, 0x00); 
    qmc5883L_register_write_byte(QMC5883L_FBR, 0x01); 
}

void qmc5883l_read_xyz(t_sQMC5883L *p)
{
    uint8_t status, data_ready=0;
    int16_t mag_reg[3];

    qmc5883L_register_read(QMC5883L_STATUS, &status, 1); // 读状态寄存器 

    if (status & 0x01)
    {
        data_ready = 1;
    }
    if (data_ready == 1)
    {
        data_ready = 0;
        qmc5883L_register_read(QMC5883L_XOUT_L, (uint8_t *)mag_reg, 6);

        p->mag_x = mag_reg[0];
        p->mag_y = mag_reg[1];
        p->mag_z = mag_reg[2];
        //p->azimuth = (float)atan2(p->mag_y, p->mag_x) * 180.0 / 3.1415926 + 180.0;
    }
}

void qmc5883l_fetch_azimuth(t_sQMC5883L *p)
{
    qmc5883l_read_xyz(p);

    p->azimuth = (float)atan2(p->mag_y, p->mag_x) * 180.0 / 3.1415926 + 180.0;
}

