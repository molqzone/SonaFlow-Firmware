#ifndef APP_LED_MANAGER_HPP_
#define APP_LED_MANAGER_HPP_

#include <memory>
#include "esp_err.h"
#include "led_strip.h"

namespace led {

/**
 * @class LEDManager
 * @brief A singleton manager for controlling an addressable LED strip.
 *
 * This class provides a high-level C++ interface for the led_strip
 * component. It correctly models the two-step process of updating a buffer
 * (SetPixelColor) and then transmitting the buffer to the physical LEDs
 * (Refresh).
 */
class LEDManager {
   public:
    struct Config {
        int gpio_pin;
        uint32_t max_leds = 1;
        uint32_t resolution_hz = 10 * 1000 * 1000;  // 10MHz
    };

    // --- Singleton Management ---
    LEDManager(const LEDManager&) = delete;
    LEDManager& operator=(const LEDManager&) = delete;
    ~LEDManager();

    static esp_err_t CreateInstance(const Config& config);
    static LEDManager& GetInstance();

    // --- Public Control Methods ---

    /**
     * @brief Sets the color of a single LED and immediately displays it.
     *
     * This is the most common use case. It's a convenient helper that calls
     * SetPixelColor() followed by Refresh().
     *
     * @param index The index of the LED to set (0-based).
     * @param red The red component of the color (0-255).
     * @param green The green component of the color (0-255).
     * @param blue The blue component of the color (0-255).
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t SetAndRefreshColor(uint32_t index, uint8_t red, uint8_t green,
                                 uint8_t blue);

    /**
     * @brief Turns off all LEDs immediately.
     *
     * A convenient helper that calls led_strip_clear(). Since clear() also
     * refreshes the strip, the change is immediate.
     *
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t TurnOff();

    // --- Advanced Control Methods ---

    /**
     * @brief Updates the color of a single LED in the internal buffer only.
     *
     * Use this method to stage changes for multiple LEDs before displaying them
     * all at once with a single call to Refresh().
     *
     * @note This method does NOT change the physical LED's color. You must call
     * Refresh() to apply the change.
     */
    esp_err_t SetPixelColor(uint32_t index, uint8_t red, uint8_t green,
                            uint8_t blue);

    /**
     * @brief Transmits the current state of the color buffer to the physical LEDs.
     */
    esp_err_t Refresh();

   private:
    LEDManager() = default;
    esp_err_t Initialize(const Config& config);

    led_strip_handle_t led_strip_handle_ = nullptr;
    Config config_{};

    static std::unique_ptr<LEDManager> s_instance_;
};

}  // namespace led

#endif  // APP_LED_MANAGER_HPP_