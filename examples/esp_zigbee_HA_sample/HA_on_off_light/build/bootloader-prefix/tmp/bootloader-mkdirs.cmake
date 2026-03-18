# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/eagle/esp/esp-idf/components/bootloader/subproject"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/tmp"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/src/bootloader-stamp"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/src"
  "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/eagle/ESP32Project/examples/esp_zigbee_HA_sample/HA_on_off_light/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
