#include "states/fatal_error_state.hpp"

#include "application.hpp"
#include "esp_log.h"

namespace {
static const char* kTag = "FatalErrorState";
}  // namespace

namespace app {

FatalErrorState::FatalErrorState(Application& context) : StateBase(context) {}

void FatalErrorState::OnEnter() {
    ESP_LOGE(kTag, "FATAL ERROR DETECTED. System is halting.");
}

void FatalErrorState::Execute() {
    // Stay in an infinite loop to halt the system.
    // This prevents any further operations and makes it clear the device has failed.
    vTaskDelay(portMAX_DELAY);
}

AppState FatalErrorState::GetStateEnum() const {
    return AppState::kFatalError;
}

}  // namespace app
