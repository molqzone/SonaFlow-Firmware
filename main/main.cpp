#include <iostream>
#include <string>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_manager.hpp"

static const char* kTag = "main";

// 接收到BLE数据包时的回调函数
void OnAudioPacketReceived(const ble::AudioPacket& packet) {
    ESP_LOGI(kTag, "Received audio packet from BLE. Sequence: %d, Payload: %d",
             packet.sequence, packet.payload);
}

// BLE连接成功时的回调函数
void OnConnected() {
    ESP_LOGI(kTag, "BLE device connected! Sending a test packet...");

    // 创建一个测试用的AudioPacket
    ble::AudioPacket test_packet;
    test_packet.header = ble::PacketConfig::kHeaderSync;
    test_packet.data_type = ble::PacketConfig::kDataTypeAudio;
    test_packet.sequence = 123;
    test_packet.timestamp = esp_log_timestamp();
    test_packet.payload = 42;
    test_packet.checksum = 0;  // 编码时会自动计算

    // 发送数据包
    esp_err_t ret =
        ble::BLEManager::GetInstance()->SendAudioPacket(test_packet);
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to send test packet, error: %s",
                 esp_err_to_name(ret));
    }
}

// BLE断开连接时的回调函数
void OnDisconnected() {
    ESP_LOGW(
        kTag,
        "BLE device disconnected. Advertising will be handled internally.");
    // 移除这里的StartAdvertising()，避免重复调用
}

// 发生错误时的回调函数
void OnError(const std::string& error_message) {
    ESP_LOGE(kTag, "BLE error: %s", error_message.c_str());
}

extern "C" void app_main() {
    ESP_LOGI(kTag, "Starting BLE Manager test...");

    // 1. 初始化 BLEManager
    int ret = ble::BLEManager::CreateInstance();
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to create BLEManager instance, aborting.");
        return;
    }

    // 2. 获取实例并设置所有回调函数
    ble::BLEManager* ble_manager = ble::BLEManager::GetInstance();
    ble_manager->SetOnConnectedCallback(OnConnected);
    ble_manager->SetOnDisconnectedCallback(OnDisconnected);
    ble_manager->SetOnAudioPacketReceivedCallback(OnAudioPacketReceived);
    ble_manager->SetOnErrorCallback(OnError);

    // 3. 在app_main中不再调用StartAdvertising()。
    //    广告会在BLE同步事件中自动启动，这更符合事件驱动编程的范式。

    // 主程序进入无限循环
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}