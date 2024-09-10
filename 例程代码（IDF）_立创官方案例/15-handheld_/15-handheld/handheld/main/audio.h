#pragma once

/* Example configurations */
#define EXAMPLE_RECV_BUF_SIZE   (2400)
#define EXAMPLE_SAMPLE_RATE     (16000)
#define EXAMPLE_MCLK_MULTIPLE   (384) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ    (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME    70   // 音量大小 取值范围0-100

#define MIC_GAIN_0DB            0  // 麦克风增益 0dB
#define MIC_GAIN_6DB            1  // 麦克风增益 6dB
#define MIC_GAIN_12DB           1  // 麦克风增益 12dB
#define MIC_GAIN_18DB           1  // 麦克风增益 18dB
#define MIC_GAIN_24DB           1  // 麦克风增益 24dB
#define MIC_GAIN_30DB           1  // 麦克风增益 30dB
#define MIC_GAIN_36DB           1  // 麦克风增益 36dB
#define MIC_GAIN_42DB           1  // 麦克风增益 42dB

/* I2C port and GPIOs */
#define I2S_NUM         (0)
#define I2S_MCK_IO      (GPIO_NUM_10)
#define I2S_BCK_IO      (GPIO_NUM_8)
#define I2S_WS_IO       (GPIO_NUM_12)
#define I2S_DO_IO       (GPIO_NUM_11)
#define I2S_DI_IO       (GPIO_NUM_7)


extern void audio_init(void);
extern void power_music_task(void *args);
extern void echo_task(void *args);

