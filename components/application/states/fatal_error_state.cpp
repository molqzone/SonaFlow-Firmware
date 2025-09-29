#include "states/fatal_error_state.hpp"

#include "application.hpp"
#include "esp_log.h"
#include "led_manager.hpp"

namespace {
static const char* kTag = "FatalErrorState";
constexpr uint32_t kErrorBlinkPeriodMs = 250;
}  // namespace

namespace app {

FatalErrorState::FatalErrorState(Application& context) : StateBase(context) {}

void FatalErrorState::OnEnter() {
    ESP_LOGE(kTag, "FATAL ERROR DETECTED. System is halting.");
    led_on_ = true;
}

void FatalErrorState::Execute() {
    if (led_on_) {
        // If the state is 'on', set the LED to red.
        led::LEDManager::GetInstance().SetAndRefreshColor(0, 255, 0, 0);  // Red
    } else {
        // If the state is 'off', turn the LED off.
        led::LEDManager::GetInstance().TurnOff();
    }

    // Invert the state for the next call.
    led_on_ = !led_on_;

    // Wait for the defined period. This short, periodic delay creates the
    // blinking effect and allows the RTOS to run other tasks if needed.
    vTaskDelay(pdMS_TO_TICKS(kErrorBlinkPeriodMs));
}

AppState FatalErrorState::GetStateEnum() const {
    return AppState::kFatalError;
}

}  // namespace app
