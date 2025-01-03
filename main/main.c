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

// Dirección MAC base personalizada
uint8_t custom_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x51, 0xE4}; // Dirección pública válida

// UUIDs en el orden correcto
static uint8_t service_uuid_on[16] = {0x00, 0x01, 0x00, 0xff, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
static uint8_t service_uuid_off[16] = {0x00, 0x01, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};

// Callback de eventos GAP BLE
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Anuncio BLE iniciado correctamente.");
        } else {
            ESP_LOGE(TAG, "Fallo al iniciar el anuncio BLE: %d", param->adv_start_cmpl.status);
        }
        break;
    default:
        break;
    }
}

// Función para configurar el anuncio BLE
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
        ESP_LOGE(TAG, "Fallo al configurar los datos de anuncio BLE: %s", esp_err_to_name(ret));
    }
}

void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configurar dirección MAC personalizada
    ret = esp_base_mac_addr_set(custom_mac);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Dirección MAC base personalizada configurada correctamente: %02X:%02X:%02X:%02X:%02X:%02X",
                 custom_mac[0], custom_mac[1], custom_mac[2], custom_mac[3], custom_mac[4], custom_mac[5]);
    } else {
        ESP_LOGE(TAG, "Error al configurar la dirección MAC base personalizada: %s", esp_err_to_name(ret));
        return;
    }

    // Inicializar controlador Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Fallo al inicializar el controlador BT: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM); // Cambiado a modo Dual
    if (ret) {
        ESP_LOGE(TAG, "Fallo al habilitar el controlador BT: %s", esp_err_to_name(ret));
        return;
    }

    // Inicializar Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Fallo al inicializar Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Fallo al habilitar Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    // Registrar callback GAP BLE
    esp_ble_gap_register_callback(gap_event_handler);

    // Anunciar UUIDs alternados cada 5 segundos
    while (1) {
        ESP_LOGI(TAG, "Enviando UUID de encendido...");
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

        ESP_LOGI(TAG, "Enviando UUID de apagado...");
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