
# Zigbee "Ruchka" Device (ESP32-C6)

Проект реализует управление "умной" оконной ручкой (делали на базе кластера ESP Window Covering, для того, чтобы можно было управлять устройством из приложения Умный Дом от яндекса) на базе микроконтроллера **ESP32-C6** с использованием стека **Zigbee (ZCL)** и двух сервоприводов. Устройство работает как Zigbee End Device, управляя положением окна по командам из Zigbee-сети.

## Почему esp-idf:

Официальный Zigbee-стек — Arduino не имеет встроенной поддержки Zigbee, особенно на ESP32-C6. Придётся использовать сырые форки. ESP-IDF даёт сертифицированный esp-zigbee-sdk от Espressif с кластером Window Covering — работает с любым координатором (ZHA, Zigbee2MQTT, Hue) без плясок.

Полный контроль над железом — драйверы сервоприводов через аппаратные таймеры (LEDC), точная настройка таймингов, прямое управление регистрами. В Arduino digitalWrite — медленный прослоёчный костыль.

Отладка как у взрослых — паника с сохранением стека, монитор задач, трассировка Zigbee, heap poisoning, watchdogs. Arduino ответит просто перезагрузкой — ищите причину в чёрном ящике.

Коротко: Arduino — для мигания светодиодом на столе. ESP-IDF — для устройства, которое живёт в вашей сети месяцами, не зависает, не жрёт батарейку и не позорит вас перед домашними, когда окно не закрывается. Вот и думайте

## Требования

- **Аппаратное обеспечение**: плата с ESP32-C6.
- **Операционная система**: Linux, macOS, Windows (с WSL2) – я делал на Ubuntu 20.04/22.04 в WSL2, с другими осями быть проблем не должно.
- **ESP-IDF** версии **5.4.2** (пояснение ниже).
- Утилиты: `git`, `make`, `python3`, `pip`, `screen` (опционально).

## Выбор версии ESP-IDF: почему 5.4.2?

Для стабильной работы **esp-zigbee-sdk** (официального SDK от Espressif для Zigbee) требуется строго определённая версия ESP-IDF.  

- **ESP-IDF v6.0** содержит критические изменения API (breaking changes), из-за которых сборка esp-zigbee-sdk может завершиться ошибкой.  
- **ESP-IDF v5.4.2** официально протестирована и гарантированно работает с esp-zigbee-sdk на ESP32-C6.  

Поэтому в проекте используется именно **v5.4.2** – проверенная и надёжная версия.

> **Примечание:** ESP32-C6 поддерживается начиная с ESP-IDF v5.0, а v5.4.2 – это стабильный релиз с полной поддержкой Zigbee.

## Установка ESP-IDF 5.4.2

### Linux / macOS / WSL

1. **Создайте папку для установки** (например, `~/esp`):
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   ```

2. **Клонируйте репозиторий ESP-IDF и переключитесь на ветку v5.4.2**:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   git checkout v5.4.2
   git submodule update --init --recursive
   ```

3. **Запустите инсталлятор** (установит инструменты, компилятор, CMake и т.д.):
   ```bash
   ./install.sh
   ```

4. **Активируйте окружение** (его нужно не забывать активировать каждый раз при открытии нового терминала):
   ```bash
   . ~/esp/esp-idf/export.sh
   ```

### Windows (с использованием WSL2)

Рекомендуется использовать WSL2 (Ubuntu) и выполнить все шаги для Linux внутри WSL. Для работы с последовательным портом и прошивкой потребуется настроить проброс USB из Windows в WSL (см. раздел [Проброс USB-устройств](#windows-wsl2--проброс-usb-устройств)).

### Создание alias для быстрой активации окружения

Чтобы каждый раз не вводить `. ~/esp/esp-idf/export.sh`, создайте alias в файле `~/.bashrc` (или `~/.zshrc` для Zsh):
```bash
echo "alias get_idf='. ~/esp/esp-idf/export.sh'" >> ~/.bashrc
source ~/.bashrc
```
Теперь для активации окружения ESP-IDF в любом новом терминале достаточно выполнить:
```bash
get_idf
```

## Клонирование и сборка проекта

1. **Клонируйте репозиторий проекта**:
   ```bash
   git clone https://github.com/EagleIX/ESP32Project
   cd HA_ZB_Project
   ```

2. **Активируйте окружение ESP-IDF**:
   ```bash
   get_idf
   ```

3. **Установите целевой чип**:
   ```bash
   idf.py set-target esp32c6
   ```

4. **Загрузите компоненты** (проект использует `idf_component.yml` – зависимости подтянутся автоматически):
   ```bash
   idf.py reconfigure
   ```

5. **Соберите проект**:
   ```bash
   idf.py build
   ```

## Прошивка и мониторинг

### Linux / macOS / WSL (прямое подключение)

1. Подключите ESP32-C6 через USB. Узнайте имя порта (например, `/dev/ttyUSB0`, `/dev/ttyACM0`):
   ```bash
   ls /dev/ttyUSB* /dev/ttyACM*
   ```

2. **Прошивка**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash
   ```

3. **Запуск монитора** (последовательный вывод):
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```
   Выход из монитора: `Ctrl+]`

### Windows (WSL2) – проброс USB-устройств

Если вы работаете в WSL2, USB-порт ESP32-C6 будет недоступен напрямую. Необходимо выполнить проброс (forwarding) устройства из Windows в WSL.

1. **Установите в Windows утилиту `usbipd-win`**:
   - Скачайте последнюю версию с [GitHub](https://github.com/dorssel/usbipd-win).
   - Установите и запустите службу от имени администратора.

2. В PowerShell (от администратора) **определите BUSID вашего устройства**:
   ```powershell
   usbipd list
   ```
   Найдите строку с описанием вашей платы ESP32-C6 (например, Silicon Labs CP210x или Espressif USB JTAG/serial). Запомните BUSID (например, `2-1`).

3. **Опубликуйте (bind) устройство** (сделайте доступным для WSL):
   ```powershell
   usbipd bind --busid 2-1
   ```

4. **Присоедините (attach) устройство к WSL**:
   ```powershell
   usbipd attach --wsl --busid 2-1
   ```

5. Внутри WSL теперь должно появиться устройство `/dev/ttyACM0` или `/dev/ttyUSB0`. Проверьте:
   ```bash
   ls -la /dev/ttyACM*
   ```

6. Прошейте и запустите монитор стандартными командами:
   ```bash
   idf.py -p /dev/ttyACM0 flash monitor
   ```

7. **Отключение устройства от WSL** (когда не нужно):
   ```powershell
   # В PowerShell от администратора
   usbipd detach --busid 2-1
   ```

## Очистка и пересборка

Если сборка пошла не так, выполните полную очистку:
```bash
# 1. Очистить артефакты сборки
idf.py clean
idf.py fullclean

# 2. Стереть flash (опционально)
idf.py erase-flash

# 3. Пересобрать с нуля
idf.py build

# 4. Прошить
idf.py -p /dev/ttyUSB0 flash
```

## Структура проекта

```
WindowCovering_sample/
├── HA_Window_covering/           
├── main/                         # Основной код приложения
│   ├── CMakeLists.txt
│   ├── esp_zb_window_covering.c  # Zigbee кластер Window Covering
│   ├── esp_zb_window_covering.h
│   ├── idf_component.yml         # Зависимости компонента main
│   ├── servo_driver.c            # Драйвер сервоприводов
│   └── servo_driver.h
├── managed_components/           # Управляемые компоненты (ESP-IDF)
├── CMakeLists.txt                # Корневой CMake
├── dependencies.lock             # Зафиксированные версии зависимостей
├── partitions.csv                # Таблица разделов flash
├── sdkconfig                     # Текущая конфигурация сборки
├── sdkconfig.defaults            # Настройки по умолчанию
└── sdkconfig.old                 # Предыдущая конфигурация (бэкап)
```

## Устранение неполадок

### Ошибка "Idf.py not found"
- Активируйте окружение: `get_idf` (или `. ~/esp/esp-idf/export.sh`).

### Не удаётся найти порт в WSL
- Убедитесь, что устройство привязано и подключено через `usbipd`.
- Попробуйте отключить и снова подключить USB-кабель.

### Ошибка сборки из-за версии ESP-IDF
- Проверьте версию: `idf.py --version` → должно быть `v5.4.2`.
- Если версия другая, переключитесь на правильную ветку в папке `~/esp/esp-idf`.

## Дополнительная информация

- Документация ESP-IDF: [https://docs.espressif.com/projects/esp-idf/](https://docs.espressif.com/projects/esp-idf/)
- ESP-Zigbee-SDK: [https://github.com/espressif/esp-zigbee-sdk](https://github.com/espressif/esp-zigbee-sdk)


