#ifndef APP_LED_MANAGER_HPP_
#define APP_LED_MANAGER_HPP_

#include <memory>
#include "esp_err.h"
#include "led_strip.h"  // Use the high-level component header

namespace led {

/**
 * @class LEDManager
 * @brief A singleton manager for controlling an addressable LED strip (e.g., WS2812).
 *
 * This class provides a high-level C++ interface for initializing and
 * controlling an LED strip. Its implementation is based on the official ESP-IDF
 * led_strip component example, using the RMT backend.
 */
class LEDManager {
   public:
    /**
   * @struct Config
   * @brief Configuration structure for the LEDManager.
   */
    struct Config {
        int gpio_pin;
        uint32_t max_leds = 1;
        uint32_t resolution_hz =
            10 * 1000 * 1000;  // 10MHz, a common resolution
    };

    // --- Singleton Management ---
    LEDManager(const LEDManager&) = delete;
    LEDManager& operator=(const LEDManager&) = delete;
    ~LEDManager();

    /**
   * @brief Creates and initializes the unique LEDManager instance.
   * @param config The configuration for the LED strip.
   * @return esp_err_t ESP_OK on success.
   */
    static esp_err_t CreateInstance(const Config& config);

    /**
   * @brief Gets the singleton instance of the LEDManager.
   * @warning CreateInstance() must have been called successfully before this.
   * @return A reference to the unique LEDManager instance.
   */
    static LEDManager& GetInstance();

    // --- Public Control Methods ---

    /**
   * @brief Sets the color of a specific LED in the strip's buffer.
   * @note You must call Refresh() to see the change on the physical LED.
   * @param index The index of the LED to set (0-based).
   * @param red The red component of the color (0-255).
   * @param green The green component of the color (0-255).
   * @param blue The blue component of the color (0-255).
   * @return esp_err_t ESP_OK on success.
   */
    esp_err_t SetPixelColor(uint32_t index, uint8_t red, uint8_t green,
                            uint8_t blue);

    /**
   * @brief Refreshes the LED strip to display the latest colors set in the buffer.
   * @return esp_err_t ESP_OK on success.
   */
    esp_err_t Refresh();

    /**
   * @brief Clears all LEDs in the strip (turns them off).
   *
   * This is a convenient helper that clears the buffer and then calls Refresh().
   * @return esp_err_t ESP_OK on success.
   */
    esp_err_t Clear();

   private:
    LEDManager() = default;
    esp_err_t Initialize(const Config& config);

    led_strip_handle_t led_strip_handle_ = nullptr;
    Config config_{};  // Store the configuration for internal use

    static std::unique_ptr<LEDManager> s_instance_;
};

}  // namespace led

#endif  // APP_LED_MANAGER_HPP_