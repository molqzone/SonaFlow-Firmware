#include "states/uninitialized_state.hpp"
#include "freertos/FreeRTOS.h"

namespace app {

UninitializedState::UninitializedState(Application& context)
    : StateBase(context) {}

// This state does nothing, waiting for the system to formally set the first
// operational state.
void UninitializedState::Execute() {
    vTaskDelay(portMAX_DELAY);
}

AppState UninitializedState::GetStateEnum() const {
    return AppState::kUninitialized;
}

}  // namespace app