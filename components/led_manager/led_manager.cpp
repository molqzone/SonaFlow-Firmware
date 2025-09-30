#include "led_manager.hpp"
#include <cassert>
#include "esp_log.h"

namespace {
static const char* kTag = "LedManager";
}

namespace led {

std::unique_ptr<LEDManager> LEDManager::s_instance_ = nullptr;

esp_err_t LEDManager::CreateInstance(const LEDManager::Config& config) {
    if (s_instance_ != nullptr) {
        return ESP_OK;
    }
    s_instance_ = std::unique_ptr<LEDManager>(new LEDManager());
    esp_err_t err = s_instance_->Initialize(config);
    if (err != ESP_OK) {
        s_instance_.reset();
    }
    return err;
}

LEDManager& LEDManager::GetInstance() {
    assert(s_instance_ != nullptr);
    return *s_instance_;
}

LEDManager::~LEDManager() {
    if (led_strip_handle_) {
        led_strip_del(led_strip_handle_);
    }
}

esp_err_t LEDManager::Initialize(const LEDManager::Config& config) {
    config_ = config;
    ESP_LOGI(kTag, "Initializing LED Strip on GPIO %d", config_.gpio_pin);

    led_strip_config_t strip_config = {
        .strip_gpio_num = config_.gpio_pin,
        .max_leds = config_.max_leds,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false},
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = config_.resolution_hz,
        .mem_block_symbols = 0,
        .flags = {.with_dma = true},
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config,
                                             &led_strip_handle_));
    ESP_LOGI(kTag, "Created LED strip object successfully.");

    // Turn the LED off on initialization
    return TurnOff();
}

esp_err_t LEDManager::SetAndRefreshColor(uint32_t index, uint8_t red,
                                         uint8_t green, uint8_t blue) {
    esp_err_t ret = SetPixelColor(index, red, green, blue);
    if (ret != ESP_OK) {
        return ret;
    }
    return Refresh();
}

esp_err_t LEDManager::TurnOff() {
    if (!led_strip_handle_) {
        return ESP_ERR_INVALID_STATE;
    }
    // led_strip_clear() updates the buffer and refreshes the strip.
    return led_strip_clear(led_strip_handle_);
}

esp_err_t LEDManager::SetPixelColor(uint32_t index, uint8_t red, uint8_t green,
                                    uint8_t blue) {
    if (!led_strip_handle_ || index >= config_.max_leds) {
        return ESP_ERR_INVALID_ARG;
    }
    return led_strip_set_pixel(led_strip_handle_, index, red, green, blue);
}

esp_err_t LEDManager::Refresh() {
    if (!led_strip_handle_) {
        return ESP_ERR_INVALID_STATE;
    }
    return led_strip_refresh(led_strip_handle_);
}

}  // namespace led