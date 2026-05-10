/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier:  LicenseRef-Included
 *
 * Zigbee HA_on_off_light Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Вспомогательные макросы ESP для проверок и логирования
#include "esp_check.h"           // ESP_RETURN_ON_FALSE, ESP_ERROR_CHECK
#include "esp_log.h"             // ESP_LOGI, ESP_LOGW, определение TAG
#include "nvs_flash.h"           // Инициализация энергонезависимой памяти (хранит ключи, сетевые параметры)

// Zigbee стек от Espressif: стандартные профили Home Automation
#include "ha/esp_zigbee_ha_standard.h"

// Локальный заголовочный файл, содержащий конфигурации и объявления для данного устройства
#include "esp_zb_window_control.h"

//Локальный заголовочный файл для драйвера сервоприводов
#include "servo_driver.h"


// Принудительная проверка: данный пример предназначен для устройства типа End Device (спящее устройство).
#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif
// Тег для вывода логов (виден в мониторе последовательного порта)
static const char *TAG = "HA_ESP_WINDOW_COVEWRING";



/********************* Define functions **************************/
/**
 * @brief Отложенная инициализация драйвера освещения.
 *        Вызывается после того, как Zigbee стек сообщил об успешном старте устройства.
 * @return Всегда ESP_OK (упрощённо; в реальном драйвере может быть ошибка).
 */

static esp_err_t deferred_driver_init(void)
{
    servo_driver_init();
    return ESP_OK;
}
/**
 * @brief Функция обратного вызова (callback) для повторного запуска процедуры ввода в сеть (commissioning).
 *        Используется вместе с esp_zb_scheduler_alarm для повторных попыток.
 * @param mode_mask Режим комиссионирования (например, ESP_ZB_BDB_MODE_NETWORK_STEERING)
 */
static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    // ESP_RETURN_ON_FALSE проверяет условие, при ложности – логирует ошибку и выходит.
    // Здесь просто запускается верхнеуровневая комиссионирование без проверки возврата (пустое тело для ";").
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}


/**
 * @brief Главный обработчик сигналов (событий) Zigbee стека.
 *        Регистрируется автоматически при инициализации; вызывается при изменении состояния сети, комиссионировании и т.д.
 * @param signal_struct Структура, содержащая тип сигнала и статус ошибки.
 */
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    // Указатель на код сигнала (uint32_t)
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    // Статус выполнения последней операции
    esp_err_t err_status = signal_struct->esp_err_status;
    // Тип сигнала (получаем разыменованием указателя)
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    // Сигнал, генерируемый ZDO (Zigbee Device Object) при пропуске нормального запуска.
        // Указывает, что нужно самостоятельно инициализировать стек.
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
    // Запускаем начальную инициализацию (считывание сетевых параметров из NVS и т.п.)
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    // Сигнал первого запуска устройства (чистый flash) или перезагрузки после сохранения сети.
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            // Инициализируем драйвер лампы 
            ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            // Проверяем, является ли устройство "заводски новым" (нет сохранённой сети)
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                // Запускаем поиск и присоединение к существующей Zigbee-сети (Network Steering)
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
                 // Устройство уже было в сети, просто перезагрузилось; никаких дополнительных действий не требуется.
            }
        } else {
            // Ошибка на этапе инициализации Zigbee стека
            /* commissioning failed */
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    // Сигнал, завершающий процесс Network Steering (удачный или нет)
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            // Успешное присоединение к сети – выводим информацию о параметрах сети
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        } else {
            // Присоединение не удалось – планируем повтор через 1 секунду (1000 мс)
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}


/**
 * @brief Обработчик события изменения атрибута (Set Attribute Value).
 *        Вызывается, когда координатор или другое устройство записывает значение в атрибут нашего кластера.
 * @param message Указатель на структуру сообщения об изменении атрибута.
 * @return ESP_OK при успешной обработке, иначе код ошибки.
 */
static esp_err_t zb_attribute_handler(const esp_zb_zcl_window_covering_movement_message_t *message)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Error status(%d)", message->info.status);

    ESP_LOGI(TAG, "Received: cluster 0x%x, command 0x%x, data size %d", 
             message->info.cluster, message->command, message->payload.lift_value);

    if (message->info.dst_endpoint == HA_ESP_WINDOW_COVEWRING_ENDPOINT) {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_WINDOW_COVERING) {
            switch (message->command) {
                case 0x02:   // открыть
                    ESP_LOGI(TAG, "Open command: both servos open sequentially");
                    servo_driver_open_both_sequential();
                    break;

                case 0x01:   // закрыть
                    ESP_LOGI(TAG, "Close command: both servos close sequentially");
                    servo_driver_close_both_sequential();
                    break;

                case 0x05:   // открыть на процент
                    ESP_LOGI(TAG, "Move to %d%%: Servo1 full open, Servo2 to percent", message->payload.lift_value);
                    servo_driver_move_with_percentage(message->payload.lift_value);
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown command: 0x%x", message->command);
                    break;
            }
        }
    }
    return ret;
}

/**
 * @brief Диспетчер действий (action handler) для всех callback-событий Zigbee Core.
 * @param callback_id Идентификатор типа действия (например, установка атрибута, запрос на чтение и др.)
 * @param message     Указатель на данные, связанные с действием.
 * @return ESP_OK или ошибка.
 */
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        // Пришла команда на изменение атрибута – передаём в специализированный обработчик
        ret = zb_attribute_handler((esp_zb_zcl_window_covering_movement_message_t *)message);
        break;
    case ESP_ZB_CORE_WINDOW_COVERING_MOVEMENT_CB_ID:  // 0x30 - получена команда
        // Обработка принятых команд (включая команды для window covering)
        ret = zb_attribute_handler((esp_zb_zcl_window_covering_movement_message_t *)message);
        break;
    default:
        // Другие типы callback (например, чтение атрибута, команда) – просто логируем
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

/**
 * @brief Задача FreeRTOS, в которой работает весь Zigbee стек.
 * @param pvParameters Не используется.
 */
static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack */
    /* 1. Инициализация стека Zigbee с конфигурацией End Device (ZED) */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);
    /* 2. Создание конечной точки (endpoint)
     *    Макрос ESP_ZB_DEFAULT_WINDOW_COVERING_CONFIG предоставляет стандартные настройки:
     *    - Включает кластеры: Basic, Identify, Groups, Scenes, On/off.
     */

    esp_zb_window_covering_cfg_t window_covering_cfg = ESP_ZB_DEFAULT_WINDOW_COVERING_CONFIG()  ;
    window_covering_cfg.window_cfg.covering_type = 0x00; 
    esp_zb_ep_list_t *esp_zb_window_covering_ep = esp_zb_window_covering_ep_create(HA_ESP_WINDOW_COVEWRING_ENDPOINT , &window_covering_cfg);
    /* 3. Добавление пользовательской информации о производителе и модели в кластер Basic */
    zcl_basic_manufacturer_info_t info = {
        .manufacturer_name = ESP_MANUFACTURER_NAME,
        .model_identifier = ESP_MODEL_IDENTIFIER,
    };

    esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_window_covering_ep, HA_ESP_WINDOW_COVEWRING_ENDPOINT, &info);

    /* 4. Регистрация созданного endpoint'а в Zigbee стеке */
    esp_zb_device_register(esp_zb_window_covering_ep);

    /* 5. Регистрация единого обработчика всех действий (callback) */
    esp_zb_core_action_handler_register(zb_action_handler);

    /* 6. Установка маски первичных каналов (например, все каналы) */
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

    /* 7. Запуск Zigbee стека.
     *    Параметр false означает, что BDB commissioning не стартует автоматически,
     *    а будет запущен позже через сигналы (ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP).
     */
    ESP_ERROR_CHECK(esp_zb_start(false));

    /* 8. Вход в бесконечный цикл обработки событий стека (никогда не возвращает управление) */
    esp_zb_stack_main_loop();
}


/**
 * @brief Главная функция приложения (точка входа в программу на ESP-IDF).
 */
void app_main(void)
{
    // Конфигурация платформы Zigbee: используется встроенный радиочип и прямое управление (без внешнего хоста)
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),// радио в нативном режиме
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),// соединение отсутствует (стек работает на самом ESP)
    };

    // Инициализация NVS (необходима для хранения ключей, параметров сети Zigbee)
    ESP_ERROR_CHECK(nvs_flash_init());
    // Применение платформенной конфигурации
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    // Создаём задачу FreeRTOS для работы Zigbee стека (стек задачи: 4096 байт, приоритет 5)
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
