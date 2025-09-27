#include "application.hpp"

#include <cassert>  // For assert()

#include "esp_log.h"

// Include the full definitions of the components we use.
#include "audio_source.hpp"
#include "ble_manager.hpp"

namespace {
// File-local constants.
static const char* kTag = "Application";
}  // namespace

namespace app {

// Initialize the instance pointer to nullptr.
std::unique_ptr<Application> Application::s_instance_ = nullptr;

// --- Singleton Management ---

esp_err_t Application::CreateInstance() {
    if (s_instance_ != nullptr) {
        ESP_LOGW(kTag, "Application instance already created.");
        return ESP_OK;
    }
    // Use 'new' as the constructor is private.
    s_instance_ = std::unique_ptr<Application>(new Application());

    // Perform fallible initialization.
    esp_err_t err = s_instance_->Initialize();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to initialize Application.");
        s_instance_.reset();
        return err;
    }

    ESP_LOGI(kTag, "Application instance created successfully.");
    return ESP_OK;
}

Application& Application::GetInstance() {
    // This assert ensures that CreateInstance() has been called successfully.
    // It will crash during development if used incorrectly, which is good.
    assert(s_instance_ != nullptr);
    return *s_instance_;
}

// --- Lifecycle and Task Management ---

Application::Application()
    // Initialize all state objects, passing a reference to this Application
    // instance ('*this') as the context.
    : waiting_state_(*this),
      connected_idle_state_(*this),
      streaming_state_(*this),
      fatal_error_state_(*this),
      // Set the initial state pointer to a safe default.
      current_state_(&waiting_state_) {}

esp_err_t Application::Initialize() {
    ESP_LOGI(kTag, "Initializing components...");

    // --- Initialize BLEManager Singleton ---
    esp_err_t ret = ble::BLEManager::CreateInstance();
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to create BLEManager instance.");
        return ret;
    }
    ble_manager_ = ble::BLEManager::GetInstance();

    // --- Initialize AudioSource Instance ---
    // The factory returns a unique_ptr, which we take ownership of.
    audio_source_ = audio::AudioSource::Create();
    if (!audio_source_) {
        ESP_LOGE(kTag, "Failed to create AudioSource instance.");
        return ESP_FAIL;
    }

    // --- Setup Callbacks ---
    // Use lambdas to forward the BLE events to our private handler methods.
    ble_manager_->SetOnConnectedCallback([this]() { this->OnBleConnected(); });
    ble_manager_->SetOnDisconnectedCallback(
        [this]() { this->OnBleDisconnected(); });

    ESP_LOGI(kTag, "Components initialized. Setting initial state.");

    // Formally enter the first operational state.
    SetState(AppState::kWaitingForConnection);

    return ESP_OK;
}

void Application::Start() {
    ESP_LOGI(kTag, "Starting main application task...");
    // Create the FreeRTOS task that will run the main loop.
    xTaskCreate(MainTask, "MainAppTask", 4096, this, 5, &main_task_handle_);
}

void Application::MainTask(void* param) {
    // The static task entry point simply calls the instance's run method.
    Application* app = static_cast<Application*>(param);
    app->RunMainTask();
}

void Application::RunMainTask() {
    ESP_LOGI(kTag, "Main task loop started.");
    while (true) {
        // The core of the state machine: delegate execution to the current state.
        current_state_->Execute();
    }
}

// --- State Transition and Event Handlers ---

void Application::SetState(AppState new_state_enum) {
    // Use a lock_guard for thread-safe state transitions.
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (current_state_->GetStateEnum() == new_state_enum) {
        return;  // Already in the target state.
    }

    ESP_LOGI(kTag, "Transitioning from state [%d] to [%d]",
             static_cast<int>(current_state_->GetStateEnum()),
             static_cast<int>(new_state_enum));

    current_state_->OnExit();

    switch (new_state_enum) {
        case AppState::kWaitingForConnection:
            current_state_ = &waiting_state_;
            break;
        case AppState::kConnectedIdle:
            current_state_ = &connected_idle_state_;
            break;
        case AppState::kStreamingAudio:
            current_state_ = &streaming_state_;
            break;
        case AppState::kFatalError:
            current_state_ = &fatal_error_state_;
            break;
        default:
            ESP_LOGE(kTag, "Attempted to transition to an unknown state!");
            // In case of an unknown state, transition to the error state for safety.
            current_state_ = &fatal_error_state_;
    }

    current_state_->OnEnter();
}

void Application::OnBleConnected() {
    ESP_LOGI(kTag, "Event: BLE Connected. Transitioning to ConnectedIdle.");
    // A client has connected. Transition to the idle-connected state.
    SetState(AppState::kConnectedIdle);
}

void Application::OnBleDisconnected() {
    ESP_LOGI(kTag, "Event: BLE Disconnected. Transitioning to Waiting.");
    // The client has disconnected. Go back to waiting for a new connection.
    SetState(AppState::kWaitingForConnection);
}

}  // namespace app
