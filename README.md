Here is a revised GitHub-friendly version of your content, formatted for better readability and structure: 

---

# Change MAC Address Using ESP-IDF

This repository contains a simple example using ESP-IDF to change the MAC address on an ESP32 device. This can be useful for various Bluetooth applications. 

---

## Getting Started

### Step 1: Install ESP-IDF
To get started, install the ESP-IDF. You can follow the [official ESP-IDF manual installation guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#manual-installation).

### Step 2: Set Up Your Project
Once the ESP-IDF is installed, create a new project using the following commands:

```bash
idf.py create-project my_bluetooth_project
cd my_bluetooth_project
```

Ensure your project folder structure looks like this:

```
my_bluetooth_project/
├── CMakeLists.txt
├── sdkconfig
├── main/
    ├── CMakeLists.txt
    ├── main.c
```

### Step 3: Configure CMakeLists
Edit the `CMakeLists.txt` file in the project root directory:

```cmake
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_bluetooth_project)
```

Edit the `CMakeLists.txt` file in the `main` directory:

```cmake
idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS ".")
```

---

## Code Example
Below is the content of the `main.c` file:

```c
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "nvs_flash.h"
#include "esp_mac.h"

static const char *TAG = "MAC_CUSTOM_BT";

uint8_t custom_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x51, 0xE4};

static uint8_t service_uuid_on[16] = {0x00, 0x01, 0x00, 0xff, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
static uint8_t service_uuid_off[16] = {0x00, 0x01, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "BLE advertisement started successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to start BLE advertisement: %d", param->adv_start_cmpl.status);
            }
            break;
        default:
            break;
    }
}

void configure_ble_adv_data(uint8_t *uuid) {
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 16,
        .p_service_uuid = uuid,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret) {
        ESP_LOGE(TAG, "Failed to configure BLE advertisement data: %s", esp_err_to_name(ret));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = esp_base_mac_addr_set(custom_mac);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Custom base MAC address configured successfully: %02X:%02X:%02X:%02X:%02X:%02X",
                 custom_mac[0], custom_mac[1], custom_mac[2], custom_mac[3], custom_mac[4], custom_mac[5]);
    } else {
        ESP_LOGE(TAG, "Error configuring custom base MAC address: %s", esp_err_to_name(ret));
        return;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize BT controller: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable BT controller: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    esp_ble_gap_register_callback(gap_event_handler);

    while (1) {
        ESP_LOGI(TAG, "Sending UUID On...");
        configure_ble_adv_data(service_uuid_on);
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .peer_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "Sending UUID Off...");
        configure_ble_adv_data(service_uuid_off);
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .peer_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

---
### Step 4: Configurate your menuconfig

Now we are gonna execute on bash ESP-IDF 5.3 CMD

Set our target to esp32 or your type of board
```bash
idf.py set-target esp32
```

After we set our target board we are gonna config our menu
```bash
idf.py menuconfig
```
Here we are gonna find many configuration but we only need to check the next configurations which can be seen on the screenshots right after:

### Screenshots
![Captura de pantalla 2025-01-03 055935](https://github.com/user-attachments/assets/6b0e98a7-bc4d-44e9-bf8c-ccffc10e4816)
![Captura de pantalla 2025-01-03 055923](https://github.com/user-attachments/assets/a2f33e95-dc4f-469e-8871-9dac711b98ed)
![Captura de pantalla 2025-01-03 055911](https://github.com/user-attachments/assets/4fe63c46-0999-40fc-8277-64aec38deaac)
![Captura de pantalla 2025-01-03 055850](https://github.com/user-attachments/assets/8c3d9cdc-a73f-495e-bb6a-bed27043fba4)
![Captura de pantalla 2025-01-03 055837](https://github.com/user-attachments/assets/9198d8b1-07c6-4532-a11f-e6a29454c031)
![Captura de pantalla 2025-01-03 055806](https://github.com/user-attachments/assets/b231fd8b-ad37-41a6-8295-1d248ecd6a7e)
![Captura de pantalla 2025-01-03 055754](https://github.com/user-attachments/assets/466a612b-e90b-49a0-b8c9-f54379d983b2)
![Captura de pantalla 2025-01-03 055740](https://github.com/user-attachments/assets/c4760eb6-bc19-4ba3-a54b-cdf5d0c6254b)


---

### Step 5: Build and flash

Now we are gonna execute on bash ESP-IDF 5.3 CMD

We are gona flash our project whit the next commands:
Check your port on "devices manager"

ONLY FOR FLASH KNOW PORT
```bash
idf.py -p COM(x) flash
```
ONLY FOR FLASH AUTODETECT PORT
```bash
idf.py flash
```
ONLY FOR MONITOR
```bash
idf.py -p COM(x) monitor
```
ONLY FOR MONITOR AUTODETECT PORT
```bash
idf.py flash
```
If you have issues on monitor set up baud on 115200
```bash
idf.py -p COM(x) --baud 115200 flash monitor
```

### If you need more info about check ESP-IDF GITHUB REPOSITORY

[ESP-IDF-GITHUB](https://github.com/espressif/esp-idf)

### THANKS FOR READING HOPE IT COULD HELP SOMEONE
### License
This project is licensed under the Apache 0.2 License.

---
