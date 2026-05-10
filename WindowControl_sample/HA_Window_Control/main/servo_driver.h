#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация двух сервоприводов
 *        GPIO21 – серво №1 , GPIO22 – серво №2 
 */
void servo_driver_init(void);

/**
 * @brief Установка угла для указанного серво
 * @param servo_id 0 – первый серво (GPIO21), 1 – второй (GPIO22)
 * @param angle угол в градусах (0 … SERVO_MAX_ANGLE)
 */
void servo_driver_set_angle(uint8_t servo_id, float angle);

/**
 * @brief Открыть серво №1 полностью
 */
void servo_driver_open_first(void);

/**
 * @brief Закрыть серво №1 полностью
 */
void servo_driver_close_first(void);

/**
 * @brief Открыть серво №2 полностью
 */
void servo_driver_open_second(void);

/**
 * @brief Закрыть серво №2 полностью
 */
void servo_driver_close_second(void);

/**
 * @brief Последовательное открытие: сначала серво №1, затем серво №2
 */
void servo_driver_open_both_sequential(void);

/**
 * @brief Последовательное закрытие: сначала серво №2, затем серво №1
 */
void servo_driver_close_both_sequential(void);

/**
 * @brief Специальное движение:
 *        серво №1 открывается полностью,
 *        серво №2 открывается на заданный процент (0…100)
 * @param percent процент для второго серво
 */
void servo_driver_move_with_percentage(uint8_t percent);

/**
 * @brief Установить положение сервы №2 в процентах
 * @param percent 0 – закрыто, 100 – полностью открыто
 */
void servo_driver_set_second_percentage(uint8_t percent);

#ifdef __cplusplus
}
#endif