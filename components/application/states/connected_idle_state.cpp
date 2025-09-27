#include "states/connected_idle_state.hpp"

#include "application.hpp"  // Required for context access and SetState
#include "esp_log.h"

namespace {
static const char* kTag = "ConnectedIdleState";
}  // namespace

namespace app {

ConnectedIdleState::ConnectedIdleState(Application& context)
    : StateBase(context) {}

void ConnectedIdleState::OnEnter() {}

void ConnectedIdleState::Execute() {
    ESP_LOGI(kTag, "Transitioning to Streaming state from main loop...");
    context_.SetState(AppState::kStreamingAudio);

    // 切换后，可以加一个短暂延时，让状态切换的日志有机会被打印出去。
    vTaskDelay(pdMS_TO_TICKS(10));
}

AppState ConnectedIdleState::GetStateEnum() const {
    return AppState::kConnectedIdle;
}

}  // namespace app
