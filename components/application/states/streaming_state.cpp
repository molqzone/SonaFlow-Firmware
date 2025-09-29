#include "states/streaming_state.hpp"

#include "esp_log.h"
#include "esp_timer.h"

#include "application.hpp"
#include "audio_source.hpp"
#include "ble_manager.hpp"
#include "ble_packet.hpp"
#include "led_manager.hpp"

namespace {
static const char* kTag = "StreamingState";
constexpr uint32_t kStreamingTaskDelayMs = 20;
}  // namespace

namespace app {

StreamingState::StreamingState(Application& context) : StateBase(context) {}

void StreamingState::OnEnter() {
    ESP_LOGI(kTag, "Entering Streaming state.");
    // Reset the packet sequence number for the new streaming session.
    sequence_number_ = 0;

    led::LEDManager::GetInstance().SetAndRefreshColor(0, 0, 64, 0);
}

void StreamingState::Execute() {
    // --- Robustness Check ---
    // Ensure BLE is still connected.
    if (!context_.GetBleManager()->IsConnected()) {
        ESP_LOGW(kTag,
                 "BLE disconnected unexpectedly. Transitioning to wait state.");
        // Trigger a state transition.
        context_.SetState(AppState::kWaitingForConnection);
        return;  // Exit immediately to allow the transition to happen.
    }

    // --- Get Audio Feature ---
    int8_t feature = 0;
    esp_err_t ret = context_.GetAudioSource()->GetFeature(feature);

    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to get audio feature.");
        // Wait for the next cycle instead of sending a bad packet.
        vTaskDelay(pdMS_TO_TICKS(kStreamingTaskDelayMs));
        return;
    }

    // --- Construct and Send Packet ---
    ble::AudioPacket packet = {
        .header = ble::PacketConfig::kHeaderSync,
        .data_type = ble::PacketConfig::kDataTypeAudio,
        .sequence = sequence_number_++,
        .timestamp = static_cast<uint32_t>(esp_timer_get_time() / 1000),
        .payload = feature,
        .checksum = 0,  // Checksum will be calculated by the encoder.
    };
    context_.GetBleManager()->SendAudioPacket(packet);

    // --- Rate Limiting ---
    // Yield the CPU and control the data transmission rate.
    vTaskDelay(pdMS_TO_TICKS(kStreamingTaskDelayMs));
}

AppState StreamingState::GetStateEnum() const {
    return AppState::kStreamingAudio;
}

}  // namespace app
