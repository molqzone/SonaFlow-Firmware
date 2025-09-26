#include "ble_manager.hpp"

// C Standard Libraries
#include <cstring>

// ESP-IDF & FreeRTOS Headers
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"

// NimBLE Host Stack Headers
#include "host/ble_hs_adv.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

namespace ble {

// File-Scope Static Variables and Definitions

// Logging tag for this file.
static const char* kTag = "BLEManager";

// File-scope static variable to store the characteristic handle. This avoids
// C++ language issues with static initializers and private member access.
static uint16_t g_audio_characteristic_handle = 0;

// Static singleton instance pointer.
std::unique_ptr<BLEManager> BLEManager::s_instance_ = nullptr;

// --- GATT Service and Characteristic UUIDs ---
// Note: Generate your own unique 128-bit UUIDs for a production application.
static const ble_uuid128_t gatt_service_uuid =
    BLE_UUID128_INIT(0x8C, 0xF8, 0x42, 0x5C, 0x54, 0x6E, 0x4E, 0x79, 0x9A, 0x01,
                     0x49, 0x04, 0x63, 0x93, 0x22, 0xF9);

static const ble_uuid128_t gatt_char_uuid =
    BLE_UUID128_INIT(0x2A, 0x37, 0x86, 0x24, 0x4A, 0x2B, 0x45, 0x47, 0xAD, 0x93,
                     0x82, 0x6E, 0x8A, 0x43, 0xD7, 0x9B);

// --- GATT Characteristic Definition ---
static const struct ble_gatt_chr_def gatt_audio_characteristics[] = {
    {
        .uuid = (const ble_uuid_t*)&gatt_char_uuid,
        .access_cb = BLEManager::GattAccessCallback,
        .arg = nullptr,
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_audio_characteristic_handle,
        .cpfd = nullptr,
    },
    {} /* End of characteristics array */
};

// --- GATT Service Definition ---
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        // BUG FIX: This must point to the service UUID, not the characteristic UUID.
        .uuid = (const ble_uuid_t*)&gatt_service_uuid,
        .includes = nullptr,
        .characteristics = gatt_audio_characteristics,
    },
    {} /* End of services array */
};

// Singleton Management

esp_err_t BLEManager::CreateInstance() {
    if (s_instance_ != nullptr) {
        ESP_LOGW(kTag, "BLEManager instance already exists.");
        return ESP_OK;
    }

    s_instance_ = Create();
    if (s_instance_ == nullptr) {
        ESP_LOGE(kTag, "Failed to create BLEManager instance.");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = s_instance_->Initialize();
    if (ret != ESP_OK) {
        s_instance_.reset();
    }
    return ret;
}

BLEManager* BLEManager::GetInstance() {
    return s_instance_.get();
}

// Public API Methods

esp_err_t BLEManager::StartAdvertising() {
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    // Set advertising flags: general discoverable mode and BR/EDR not supported.
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Include the 128-bit service UUID in the advertisement packet.
    fields.uuids128 = (ble_uuid128_t*)gatt_services[0].uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(kTag, "Error setting advertisement data; rc=%d", rc);
        return ESP_FAIL;
    }

    // Set the full device name in the scan response packet.
    const char* device_name = "SonaFlow";
    rc = ble_svc_gap_device_name_set(device_name);
    if (rc != 0) {
        ESP_LOGE(kTag, "Error setting device name; rc=%d", rc);
        return ESP_FAIL;
    }

    // Configure advertising parameters and start the process.
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, nullptr, BLE_HS_FOREVER,
                           &adv_params, BLEManager::GapEventHandler, nullptr);
    if (rc != 0) {
        ESP_LOGE(kTag, "Error starting advertising; rc=%d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(kTag, "Advertising started successfully.");
    return ESP_OK;
}

esp_err_t BLEManager::SendAudioPacket(const AudioPacket& packet) {
    if (send_queue_ == nullptr) {
        ESP_LOGE(kTag, "Send queue is not initialized.");
        return ESP_FAIL;
    }

    std::array<uint8_t, PacketConfig::kPacketSize> encoded_data =
        PacketEncoder::Encode(packet);

    if (xQueueSend(send_queue_, &encoded_data, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(kTag, "Send queue is full.");
        if (on_error_cb_) {
            on_error_cb_("Send queue is full.");
        }
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void BLEManager::SetOnConnectedCallback(std::function<void()> callback) {
    on_connected_cb_ = std::move(callback);
}

void BLEManager::SetOnDisconnectedCallback(std::function<void()> callback) {
    on_disconnected_cb_ = std::move(callback);
}

void BLEManager::SetOnAudioPacketReceivedCallback(
    std::function<void(const AudioPacket&)> callback) {
    on_audio_packet_received_cb_ = std::move(callback);
}

void BLEManager::SetOnErrorCallback(
    std::function<void(const std::string&)> callback) {
    on_error_cb_ = std::move(callback);
}

// Private Methods

std::unique_ptr<BLEManager> BLEManager::Create() {
    return std::unique_ptr<BLEManager>(new BLEManager());
}

esp_err_t BLEManager::Initialize() {
    ESP_LOGI(kTag, "Initializing BLEManager...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "nimble_port_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure the NimBLE host with our static callback functions.
    ble_hs_cfg.reset_cb = BLEManager::OnReset;
    ble_hs_cfg.sync_cb = BLEManager::OnSync;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ret = RegisterGattServices();
    if (ret != ESP_OK) {
        return ret;
    }

    send_queue_ = xQueueCreate(
        10, sizeof(std::array<uint8_t, PacketConfig::kPacketSize>));
    if (send_queue_ == nullptr) {
        ESP_LOGE(kTag, "Failed to create send queue.");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(SendTask, "ble_send_task", 4096, this, 5,
                    &send_task_handle_) != pdPASS) {
        ESP_LOGE(kTag, "Failed to create send task.");
        return ESP_FAIL;
    }

    if (xTaskCreate(NimbleHostTask, "nimble_host_task", 4096, nullptr, 5,
                    nullptr) != pdPASS) {
        ESP_LOGE(kTag, "Failed to create NimBLE host task.");
        return ESP_FAIL;
    }

    ESP_LOGI(kTag, "BLEManager initialized successfully.");
    return ESP_OK;
}

esp_err_t BLEManager::RegisterGattServices() {
    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_count_cfg failed; rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_add_svcs failed; rc=%d", rc);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void BLEManager::HandleGapEvent(struct ble_gap_event* event) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(kTag, "Device connected; conn_handle=%d",
                     event->connect.conn_handle);
            conn_handle_ = event->connect.conn_handle;
            if (on_connected_cb_) {
                on_connected_cb_();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(kTag, "Device disconnected; reason=%d",
                     event->disconnect.reason);
            conn_handle_ = BLE_HS_CONN_HANDLE_NONE;
            if (on_disconnected_cb_) {
                on_disconnected_cb_();
            }
            // After disconnection, restart advertising to be discoverable again.
            StartAdvertising();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(kTag, "Advertising complete; reason=%d",
                     event->adv_complete.reason);
            StartAdvertising();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(kTag,
                     "Subscribe event; conn_handle=%d, attr_handle=%d, "
                     "reason=%d, prevn=%d, curn=%d",
                     event->subscribe.conn_handle, event->subscribe.attr_handle,
                     event->subscribe.reason, event->subscribe.prev_notify,
                     event->subscribe.cur_notify);
            break;

        default:
            break;
    }
}

// Static Callbacks & Tasks (C-to-C++ Bridge)

void BLEManager::OnSync() {
    ESP_LOGI(kTag, "NimBLE host synchronized.");
    // Start advertising once the stack is ready.
    GetInstance()->StartAdvertising();
}

void BLEManager::OnReset(int reason) {
    ESP_LOGW(kTag, "NimBLE host reset; reason=%d", reason);
}

int BLEManager::GapEventHandler(struct ble_gap_event* event, void* arg) {
    // Forward the C-style event to our C++ member function handler.
    GetInstance()->HandleGapEvent(event);
    return 0;
}

int BLEManager::GattAccessCallback(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt* ctxt,
                                   void* arg) {
    // Handle write requests from the client.
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        ble::AudioPacket packet;
        if (PacketDecoder::Decode(ctxt->om->om_data, ctxt->om->om_len,
                                  packet)) {
            if (GetInstance()->on_audio_packet_received_cb_) {
                GetInstance()->on_audio_packet_received_cb_(packet);
            }
        } else {
            ESP_LOGW(kTag, "Failed to decode received packet.");
        }
    }
    return 0;  // Success
}

void BLEManager::NimbleHostTask(void* /*param*/) {
    ESP_LOGI(kTag, "NimBLE Host Task Started.");
    nimble_port_run();

    // This part is reached only when nimble_port_stop() is called.
    nimble_port_deinit();
    vTaskDelete(nullptr);
}

void BLEManager::SendTask(void* param) {
    BLEManager* manager = static_cast<BLEManager*>(param);
    std::array<uint8_t, PacketConfig::kPacketSize> packet_data;

    ESP_LOGI(kTag, "Send Task Started.");

    while (true) {
        // Block until an item is available in the queue.
        if (xQueueReceive(manager->send_queue_, &packet_data, portMAX_DELAY) ==
            pdPASS) {
            if (manager->conn_handle_ != BLE_HS_CONN_HANDLE_NONE) {
                struct os_mbuf* om = ble_hs_mbuf_from_flat(packet_data.data(),
                                                           packet_data.size());
                int rc = ble_gattc_notify_custom(
                    manager->conn_handle_, g_audio_characteristic_handle, om);
                if (rc != 0) {
                    ESP_LOGE(kTag, "Error sending notification; rc=%d", rc);
                }
            }
        }
    }
}

}  // namespace ble