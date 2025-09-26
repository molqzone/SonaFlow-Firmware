#include "audio_source.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "driver/i2s_common.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "hal/i2s_types.h"

// --- Static Hardware Configuration Constants ---
namespace {
static const char* kTag = "AudioSource";

// I2S peripheral configuration
constexpr i2s_port_t kI2sPort = I2S_NUM_AUTO;
constexpr uint32_t kI2sSampleRate = 44100;  // 44.1 kHz
constexpr i2s_data_bit_width_t kI2sBitsPerSample = I2S_DATA_BIT_WIDTH_32BIT;
constexpr uint32_t kDmaBufferCount = 16;
constexpr uint32_t kDmaBufferSamples = 1024;

// GPIO pin configuration
constexpr gpio_num_t kI2sStdGpioWs = GPIO_NUM_4;
constexpr gpio_num_t kI2sStdGpioBclk = GPIO_NUM_5;
constexpr gpio_num_t kI2sStdGpioDin = GPIO_NUM_6;

// Conversion constants
constexpr int16_t kAdcToPcmBitShift = 12;

// Limitation constants
constexpr size_t kMaxAudioSamples = 256;

// Feature Extraction Constants
// This is an empirical value representing the maximum expected RMS for scaling.
// It directly affects the sensitivity of the feature.
// **YOU MUST TUNE THIS VALUE** by observing the raw RMS output for your setup.
constexpr float kMaxRmsValue = 10000.0f;
}  // namespace

// --- Static Factory Method ---
std::unique_ptr<AudioSource> AudioSource::Create() {
    ESP_LOGI(kTag, "Attempting to create and initialize AudioSource...");

    // Configure the I2S channel
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(kI2sPort, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = kDmaBufferCount;
    chan_cfg.dma_frame_num = kDmaBufferSamples;
    chan_cfg.auto_clear = true;  // Auto clear TX buffer on underflow

    i2s_chan_handle_t rx_handle = nullptr;
    esp_err_t err = i2s_new_channel(&chan_cfg, nullptr, &rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to create I2S channel: %s",
                 esp_err_to_name(err));
        return nullptr;
    }

    // Configure the I2S standard mode
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kI2sSampleRate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(kI2sBitsPerSample,
                                                    I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = kI2sStdGpioBclk,
                .ws = kI2sStdGpioWs,
                .dout = I2S_GPIO_UNUSED,
                .din = kI2sStdGpioDin,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;  // Use left slot for mono

    // Initialize and enable the channel
    err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to initialize I2S channel in std mode: %s",
                 esp_err_to_name(err));
        i2s_del_channel(rx_handle);  // Clean up partially acquired resource
        return nullptr;
    }

    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to enable I2S channel: %s",
                 esp_err_to_name(err));
        i2s_del_channel(rx_handle);  // Clean up partially acquired resource
        return nullptr;
    }

    ESP_LOGI(kTag, "AudioSource created successfully.");

    // Use `new` because constructor is private. std::make_unique cannot access it.
    return std::unique_ptr<AudioSource>(new AudioSource(rx_handle));
}

// --- Read Method ---
esp_err_t AudioSource::Read(std::span<int16_t> dest_buffer,
                            size_t& samples_read) {
    if (!rx_handle_) {
        ESP_LOGE(kTag, "I2S channel is not initialized.");
        return ESP_FAIL;
    }

    // Validate the arguments
    if (dest_buffer.size() > kMaxAudioSamples) {
        ESP_LOGE(
            kTag,
            "Requested sample count (%zu) exceeds maximum buffer size (%zu).",
            dest_buffer.size(), kMaxAudioSamples);
        return ESP_ERR_INVALID_ARG;
    }
    if (dest_buffer.empty()) {
        samples_read = 0;
        return ESP_OK;
    }

    // Temp buffer
    std::array<int32_t, kMaxAudioSamples> bit32_buffer;
    size_t bytes_read = 0;

    esp_err_t ret = i2s_channel_read(rx_handle_, bit32_buffer.data(),
                                     dest_buffer.size() * sizeof(int32_t),
                                     &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "I2S read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    samples_read = bytes_read / sizeof(int32_t);

    // Converse the read buffer
    for (size_t i = 0; i < samples_read; i++) {
        int32_t value = bit32_buffer[i] >> kAdcToPcmBitShift;

        int32_t clamped_value =
            std::clamp(value, static_cast<int32_t>(INT16_MIN),
                       static_cast<int32_t>(INT16_MAX));
        dest_buffer[i] = static_cast<int16_t>(clamped_value);
    }

    return ESP_OK;
}

// --- Get Feature Method ---
esp_err_t AudioSource::GetFeature(int8_t& feature) {
    if (!feature) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!rx_handle_) {
        ESP_LOGE(kTag, "I2S channel is not initialized for GetFeature.");
        feature = 0;
        return ESP_FAIL;
    }

    // Use the maximum buffer size as our analysis frame.
    std::array<int16_t, kMaxAudioSamples> frame_buffer;
    size_t samples_read = 0;

    // 1. Read one full frame of audio data.
    esp_err_t ret = Read(frame_buffer, samples_read);
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to read audio frame for feature extraction: %s",
                 esp_err_to_name(ret));
        feature = 0;
        return ret;
    }

    if (samples_read == 0) {
        feature = 0;
        return ESP_OK;
    }

    // 2. Calculate the sum of squares.
    // Use a 64-bit integer to prevent overflow when squaring int16_t values.
    int64_t sum_of_squares = 0;
    for (size_t i = 0; i < samples_read; ++i) {
        // Cast to a larger type before multiplication to avoid intermediate overflow.
        int32_t sample = frame_buffer[i];
        sum_of_squares += sample * sample;
    }

    // 3. Calculate the RMS value.
    // RMS = sqrt(mean of squares)
    const double mean_square =
        static_cast<double>(sum_of_squares) / samples_read;
    const double rms_value = std::sqrt(mean_square);

    // 4. Scale the RMS value to the int8_t range [0, 127].
    // This is a linear mapping.
    double scaled_feature = (rms_value / kMaxRmsValue) * INT8_MAX;

    // 5. Clamp the value to ensure it fits within int8_t range and assign.
    double clamped_feature =
        std::clamp(scaled_feature, 0.0, static_cast<double>(INT8_MAX));
    feature = static_cast<int8_t>(clamped_feature);

    return ESP_OK;
}

// --- Private Constructor Implementation ---
AudioSource::AudioSource(i2s_chan_handle_t handle) : rx_handle_(handle) {
    ESP_LOGI(kTag, "AudioSource instance constructed.");
}

// --- Destructor ---
AudioSource::~AudioSource() {
    if (rx_handle_ != nullptr) {
        ESP_LOGI(kTag, "Disabling I2S channel...");
        ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
    }
    ESP_LOGI(kTag, "AudioSource destructed.");
}

// --- Move Constructor ---
AudioSource::AudioSource(AudioSource&& other) noexcept
    : rx_handle_(other.rx_handle_) {
    ESP_LOGI(kTag, "AudioSource move constructed.");
    other.rx_handle_ = nullptr;
}

// --- Move Assignment Operator ---
AudioSource& AudioSource::operator=(AudioSource&& other) noexcept {
    if (this != &other) {
        if (rx_handle_ != nullptr) {
            i2s_channel_disable(rx_handle_);
        }

        // Transfer ownership of the I2S handle
        rx_handle_ = other.rx_handle_;
        other.rx_handle_ = nullptr;
    }
    ESP_LOGI(kTag, "AudioSource move assigned.");
    return *this;
}
