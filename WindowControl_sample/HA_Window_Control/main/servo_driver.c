#include "servo_driver.h"
#include "iot_servo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SERVO_DRV";

// Параметры подключения двух серво
#define SERVO1_GPIO         21         // первый серво
#define SERVO2_GPIO         22         // второй серво
#define SERVO_MAX_ANGLE     180        // максимальный угол (градусы)
#define SERVO_MIN_PULSE_US  500        // мин. ширина импульса (мкс)
#define SERVO_MAX_PULSE_US   2500       // макс. ширина импульса (мкс)
#define SERVO_FREQ_HZ       50         // частота ШИМ (Гц)

// Время ожидания для полного поворота (в миллисекундах)
#define SERVO_MOVE_DELAY_MS 500

static bool s_is_initialized = false;
static ledc_mode_t s_speed_mode = LEDC_LOW_SPEED_MODE;         
static uint8_t s_channel_count = 2;
static uint8_t s_servo1_ch = 0;   // канал для первого серво
static uint8_t s_servo2_ch = 1;   // канал для второго серво

void servo_driver_init(void)
{
    if (s_is_initialized) {
        ESP_LOGW(TAG, "Servo already initialized");
        return;
    }

    servo_config_t servo_cfg = {
        .max_angle = SERVO_MAX_ANGLE,
        .min_width_us = SERVO_MIN_PULSE_US,
        .max_width_us = SERVO_MAX_PULSE_US,
        .freq = SERVO_FREQ_HZ,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = { SERVO1_GPIO, SERVO2_GPIO },
            .ch = { LEDC_CHANNEL_0, LEDC_CHANNEL_1 }
        },
        .channel_number = s_channel_count,
    };

    esp_err_t ret = iot_servo_init(s_speed_mode, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize servos: %s", esp_err_to_name(ret));
        return;
    }

    s_is_initialized = true;
    ESP_LOGI(TAG, "Both servos initialized: GPIO%d (ch%d), GPIO%d (ch%d)",
             SERVO1_GPIO, s_servo1_ch, SERVO2_GPIO, s_servo2_ch);

    // Начальное положение – закрыты
    servo_driver_close_first();
    servo_driver_close_second();
}

void servo_driver_set_angle(uint8_t servo_id, float angle)
{
    if (!s_is_initialized) {
        ESP_LOGW(TAG, "Servo not initialized");
        return;
    }
    if (servo_id >= s_channel_count) {
        ESP_LOGE(TAG, "Invalid servo_id %d", servo_id);
        return;
    }
    if (angle < 0) angle = 0;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    uint8_t channel = (servo_id == 0) ? s_servo1_ch : s_servo2_ch;
    esp_err_t ret = iot_servo_write_angle(s_speed_mode, channel, angle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set servo%d angle %.1f°", servo_id+1, angle);
    } else {
        ESP_LOGD(TAG, "Servo%d angle set to %.1f°", servo_id+1, angle);
    }
}

void servo_driver_open_first(void)
{
    servo_driver_set_angle(0, SERVO_MAX_ANGLE);
    ESP_LOGI(TAG, "Servo1 opened");
}

void servo_driver_close_first(void)
{
    servo_driver_set_angle(0, 0);
    ESP_LOGI(TAG, "Servo1 closed");
}

void servo_driver_open_second(void)
{
    servo_driver_set_angle(1, SERVO_MAX_ANGLE);
    ESP_LOGI(TAG, "Servo2 opened");
}

void servo_driver_close_second(void)
{
    servo_driver_set_angle(1, 0);
    ESP_LOGI(TAG, "Servo2 closed");
}

void servo_driver_open_both_sequential(void)
{
    ESP_LOGI(TAG, "Opening both servos sequentially: first Servo1, then Servo2");
    servo_driver_open_first();
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_DELAY_MS));
    servo_driver_open_second();
}

void servo_driver_close_both_sequential(void)
{
    ESP_LOGI(TAG, "Closing both servos sequentially: first Servo2, then Servo1");
    servo_driver_close_second();
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_DELAY_MS));
    servo_driver_close_first();
}

void servo_driver_set_second_percentage(uint8_t percent)
{
    if (percent > 100) percent = 100;
    float angle = (float)percent * SERVO_MAX_ANGLE / 100.0f;
    servo_driver_set_angle(1, angle);
    ESP_LOGI(TAG, "Servo2 set to %d%% (%.1f°)", percent, angle);
}

void servo_driver_move_with_percentage(uint8_t percent)
{
    ESP_LOGI(TAG, "Moving: Servo1 fully open, Servo2 to %d%%", percent);
    servo_driver_open_first();
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_DELAY_MS));
    servo_driver_set_second_percentage(percent);
}