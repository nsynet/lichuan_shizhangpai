#pragma once


#include <math.h>
#include "qmi8658c.h"
#include "qmc5883l.h"
#include "esp_log.h"
#include "audio.h"

#define START_MUSIC_DOWN            BIT1

extern int strength;
extern int icon_flag;
extern float temp, humi;
extern int temp_value, humi_value;


extern void get_th_task(void *args);
extern void get_att_task(void *args);


extern float bg_duty;



