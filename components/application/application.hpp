#ifndef APP_APPLICATION_HPP_
#define APP_APPLICATION_HPP_

#include <memory>
#include <mutex>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "states/connected_idle_state.hpp"
#include "states/fatal_error_state.hpp"
#include "states/state_base.hpp"
#include "states/streaming_state.hpp"
#include "states/uninitialized_state.hpp"
#include "states/waiting_state.hpp"

namespace audio {
class AudioSource;
}
namespace ble {
class BLEManager;
}

namespace app {

/**
 * @class Application
 * @brief The main application class that coordinates all system components.
 *
 * This class is a singleton that manages the application's overall state and
 * orchestrates the interaction between the AudioSource and BLEManager
 * components using a state design pattern.
 */
class Application {
   public:
    // --- Singleton Management ---
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief Creates and initializes the unique Application instance.
     * @return esp_err_t ESP_OK on success, or an ESP-IDF error code on failure.
     */
    static esp_err_t CreateInstance();

    /**
     * @brief Gets the singleton instance of the Application.
     * @warning CreateInstance() must have been called successfully before this.
     * @return A reference to the unique Application instance.
     */
    static Application& GetInstance();

    /**
     * @brief Destructor.
     */
    ~Application() = default;

    // --- Public Methods ---

    /**
     * @brief Starts the main application logic by creating the primary task.
     */
    void Start();

    /**
     * @brief Thread-safely transitions the application to a new state.
     * @param new_state The enum identifier of the state to transition to.
     */
    void SetState(AppState new_state);

    // --- Public Getters for States to Use ---
    audio::AudioSource* GetAudioSource() { return audio_source_.get(); }
    ble::BLEManager* GetBleManager() { return ble_manager_; }

   private:
    // Grant friendship to allow state classes to access the Application's
    // private members (like Getters) and methods (like SetState).
    friend class WaitingForConnectionState;
    friend class ConnectedIdleState;
    friend class StreamingState;
    friend class FatalErrorState;

    /**
     * @brief Private constructor. Initializes all state objects.
     */
    Application();

    /**
     * @brief Internal initialization method called by CreateInstance.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t Initialize();

    // --- Private Task and Event Handlers ---
    static void MainTask(void* param);
    void RunMainTask();
    void OnBleConnected();
    void OnBleDisconnected();

    // --- State Objects (pre-allocated to avoid dynamic memory) ---
    WaitingForConnectionState waiting_state_;
    ConnectedIdleState connected_idle_state_;
    StreamingState streaming_state_;
    FatalErrorState fatal_error_state_;
    UninitializedState uninitialized_state_;

    // --- Member Variables ---
    StateBase* current_state_;  // Pointer to the current active state object.
    std::mutex state_mutex_;    // Mutex to protect state transitions.

    std::unique_ptr<audio::AudioSource> audio_source_;
    ble::BLEManager* ble_manager_ = nullptr;
    TaskHandle_t main_task_handle_ = nullptr;

    // Static pointer to the single instance of this class.
    static std::unique_ptr<Application> s_instance_;
};

}  // namespace app

#endif  // APP_APPLICATION_HPP_