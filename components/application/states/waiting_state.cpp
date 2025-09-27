#include "states/waiting_state.hpp"

#include "application.hpp"
#include "audio_source.hpp"  // Assuming this needs to be included for GetAudioSource
#include "ble_manager.hpp"  // For GetBleManager
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace {
// File-local constants and variables
static const char* kTag = "WaitingState";
}  // namespace

namespace app {

WaitingForConnectionState::WaitingForConnectionState(Application& context)
    : StateBase(context) {}

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
    // In a waiting state, the main task loop has nothing to do.
    // We can add a delay to prevent this loop from spinning uselessly.
    vTaskDelay(pdMS_TO_TICKS(100));
}

AppState WaitingForConnectionState::GetStateEnum() const {
    return AppState::kWaitingForConnection;
}

}  // namespace app
