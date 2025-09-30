#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// The only application-specific header we need is our main coordinator.
#include "application.hpp"

namespace {
// File-local constants.
static const char* kTag = "app_main";
}  // namespace

// The entry point of the application must have C linkage.
extern "C" void app_main(void) {
    // --- 1. Perform low-level system initialization ---
    ESP_LOGI(kTag, "Initializing NVS flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition contains errors, erase and retry.
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(kTag, "Initializing default event loop...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // --- 2. Create and initialize the main Application instance ---
    // We use the CreateInstance method from our singleton. It handles the
    // creation and initialization of all other components (BLEManager,
    // AudioSource).
    ESP_LOGI(kTag, "Creating Application instance...");
    ret = app::Application::CreateInstance();

    // --- 3. Check for initialization success and start the application ---
    if (ret == ESP_OK) {
        ESP_LOGI(kTag, "Application created successfully. Starting...");
        // If initialization was successful, get the instance and start its main
        // task.
        app::Application::GetInstance().Start();
    } else {
        ESP_LOGE(kTag,
                 "FATAL: Failed to create Application instance. Error: %s (%d)",
                 esp_err_to_name(ret), ret);
        ESP_LOGE(kTag, "System will halt.");
        // If the core application fails to initialize, there is nothing more to
        // do. We enter an infinite loop to halt further execution. A production
        // device might trigger a reboot or a safe-mode here.
        while (true) {
            vTaskDelay(portMAX_DELAY);
        }
    }

    ESP_LOGI(kTag,
             "app_main finished. Handing over control to FreeRTOS scheduler.");
}