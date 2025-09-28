#include "states/waiting_state.hpp"

#include "application.hpp"
#include "ble_manager.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "led_manager.hpp"

namespace {
// File-local constants and variables
static const char* kTag = "WaitingState";
}  // namespace

namespace app {

WaitingForConnectionState::WaitingForConnectionState(Application& context)
    : StateBase(context), initial_led_set_(false) {}

void WaitingForConnectionState::OnEnter() {
    ESP_LOGI(kTag, "Entering Waiting for Connection state.");

    // Check advertising state
    if (!context_.GetBleManager()->IsAdvertising()) {
        ESP_LOGI(kTag, "Not advertising, starting now...");
        context_.GetBleManager()->StartAdvertising();
    } else {
        ESP_LOGW(kTag, "Already advertising, no action needed.");
    }
}

void WaitingForConnectionState::Execute() {
    if (!initial_led_set_) {
        ESP_LOGI(kTag, "Setting initial LED state...");
        led::LEDManager::GetInstance().Clear();
        esp_err_t ret =
            led::LEDManager::GetInstance().SetPixelColor(0, 0, 0, 255);
        if (ret != ESP_OK) {
            ESP_LOGE(kTag, "Failed to set LED color.");
        }
        initial_led_set_ = true;  // 设置标志位，防止重复执行
        ESP_LOGI(kTag, "Initial LED state set.");
    }

    // In a waiting state, the main task loop has nothing to do.
    // We can add a delay to prevent this loop from spinning uselessly.
    vTaskDelay(pdMS_TO_TICKS(100));
}

AppState WaitingForConnectionState::GetStateEnum() const {
    return AppState::kWaitingForConnection;
}

}  // namespace app
