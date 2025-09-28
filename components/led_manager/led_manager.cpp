#include "led_manager.hpp"

#include <cassert>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

namespace {
static const char* kTag = "LedManager";
}  // namespace

namespace led {

// Definition and initialization of the static singleton instance.
std::unique_ptr<LEDManager> LEDManager::s_instance_ = nullptr;

esp_err_t LEDManager::CreateInstance(const LEDManager::Config& config) {
    if (s_instance_ != nullptr) {
        ESP_LOGW(kTag, "LEDManager instance already created.");
        return ESP_OK;
    }
    s_instance_ = std::unique_ptr<LEDManager>(new LEDManager());
    esp_err_t err = s_instance_->Initialize(config);
    if (err != ESP_OK) {
        s_instance_.reset();  // Automatically deletes the object on failure
    }
    return err;
}

LEDManager& LEDManager::GetInstance() {
    assert(s_instance_ != nullptr);
    return *s_instance_;
}

LEDManager::~LEDManager() {
    if (led_strip_handle_) {
        ESP_LOGI(kTag, "Releasing led_strip resources.");
        // This function handles the deletion of the driver and its resources.
        led_strip_del(led_strip_handle_);
    }
}

esp_err_t LEDManager::Initialize(const LEDManager::Config& config) {
    config_ = config;  // Store the configuration
    ESP_LOGI(kTag,
             "Initializing LED Strip on GPIO %d based on the latest example",
             config_.gpio_pin);

    assert(config.gpio_pin != GPIO_NUM_NC);

    ESP_LOGI(kTag, "Resetting GPIO pin %d to default state.", config.gpio_pin);
    gpio_reset_pin(static_cast<gpio_num_t>(config.gpio_pin));

    // --- Step 1: LED strip general configuration ---
    // This setup is identical to the one in the official working example.
    led_strip_config_t strip_config = {
        .strip_gpio_num = config_.gpio_pin,
        .max_leds = config_.max_leds,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false},
    };

    // --- Step 2: RMT backend configuration ---
    // This configuration also precisely matches the example for non-DMA mode.
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = config_.resolution_hz,
        .mem_block_symbols = 0,  // Let driver choose automatically
        .flags = {
            .with_dma = true,  // Use false as per the simple example
        }};

    // --- Step 3: Create the LED strip object ---
    ESP_LOGI(kTag, "Creating LED strip object with RMT backend...");
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config,
                                             &led_strip_handle_));
    ESP_LOGI(kTag, "Created LED strip object successfully.");

    // Start with the LED turned off.
    return ESP_OK;
}

esp_err_t LEDManager::SetPixelColor(uint32_t index, uint8_t red, uint8_t green,
                                    uint8_t blue) {
    if (!led_strip_handle_ || index >= config_.max_leds) {
        return ESP_ERR_INVALID_ARG;
    }
    // This function updates the internal buffer of the driver.
    return led_strip_set_pixel(led_strip_handle_, index, red, green, blue);
}

esp_err_t LEDManager::Refresh() {
    if (!led_strip_handle_) {
        return ESP_ERR_INVALID_STATE;
    }
    // This function sends the data from the buffer to the physical LED strip.
    return led_strip_refresh(led_strip_handle_);
}

esp_err_t LEDManager::Clear() {
    if (!led_strip_handle_) {
        return ESP_ERR_INVALID_STATE;
    }
    // A convenient helper function that clears the buffer and then refreshes.
    return led_strip_clear(led_strip_handle_);
}

}  // namespace led