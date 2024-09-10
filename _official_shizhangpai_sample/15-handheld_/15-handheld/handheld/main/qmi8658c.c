#include "qmi8658c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "myi2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"

static const char *TAG = "QMI8658C";

esp_err_t qmi8658c_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, QMI8658C_SENSOR_ADDR,  &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t qmi8658c_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    return i2c_master_write_to_device(I2C_MASTER_NUM, QMI8658C_SENSOR_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void qmi8658c_init(void)
{
    uint8_t id = 0;

    qmi8658c_register_read(QMI8658C_WHO_AM_I, &id ,1);
    while (id != 0x05)
    {
        qmi8658c_register_read(QMI8658C_WHO_AM_I, &id ,1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "QMI8658C OK!");

    qmi8658c_register_write_byte(QMI8658C_RESET, 0xb0);  // 复位
    vTaskDelay(10 / portTICK_PERIOD_MS);
    qmi8658c_register_write_byte(QMI8658C_CTRL1, 0x40); // CTRL1 设置地址自动增加
    qmi8658c_register_write_byte(QMI8658C_CTRL7, 0x03); // CTRL7 允许加速度和陀螺仪
    qmi8658c_register_write_byte(QMI8658C_CTRL2, 0x95); // CTRL2 设置ACC 4g 250Hz
    qmi8658c_register_write_byte(QMI8658C_CTRL3, 0xd5); // CTRL3 设置GRY 512dps 250Hz 
}

void qmi8658c_Read_AccAndGry(t_sQMI8658C *p)
{
    uint8_t status, data_ready=0;
    int16_t buf[6];

    qmi8658c_register_read(QMI8658C_STATUS0, &status, 1); // 读状态寄存器 
    if (status & 0x03) // 判断加速度和陀螺仪数据是否可读
    {
        data_ready = 1;
    }
    if (data_ready == 1)
    {
        data_ready = 0;
        qmi8658c_register_read(QMI8658C_AX_L, (uint8_t *)buf, 12); // 读加速度值
        p->acc_x = buf[0];
        p->acc_y = buf[1];
        p->acc_z = buf[2];
        p->gyr_x = buf[3];
        p->gyr_y = buf[4];
        p->gyr_z = buf[5];
    }
}

void qmi8658c_fetch_angleFromAcc(t_sQMI8658C *p)
{
    float temp;

    qmi8658c_Read_AccAndGry(p);

    temp = (float)p->acc_x / sqrt( ((float)p->acc_y * (float)p->acc_y + (float)p->acc_z * (float)p->acc_z) );
    p->AngleX = atan(temp)*57.3f; // 180/3.14=57.3
    temp = (float)p->acc_y / sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_z * (float)p->acc_z) );
    p->AngleY = atan(temp)*57.3f; // 180/3.14=57.3
    temp = (float)p->acc_z / sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_y * (float)p->acc_y) );
    p->AngleZ = atan(temp)*57.3f; // 180/3.14=57.3
}








