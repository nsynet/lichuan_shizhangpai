#pragma once

#include <stdint.h>

#define  QMC5883L_SENSOR_ADDR    0x0D


enum qmc5883l_reg
{
    QMC5883L_XOUT_L,
    QMC5883L_XOUT_H,
    QMC5883L_YOUT_L,
    QMC5883L_YOUT_H,
    QMC5883L_ZOUT_L,
    QMC5883L_ZOUT_H,
    QMC5883L_STATUS,
    QMC5883L_TOUT_L,
    QMC5883L_TOUT_H,
    QMC5883L_CTRL1,
    QMC5883L_CTRL2,
    QMC5883L_FBR,
    QMC5883L_CHIPID = 13
};

typedef struct{
    int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
    float   azimuth;
}t_sQMC5883L;


void qmc5883l_init(void);
void qmc5883l_read_xyz(t_sQMC5883L *p);
void qmc5883l_fetch_azimuth(t_sQMC5883L *p);
