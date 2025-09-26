#include "audio_sampler.hpp"
#include <sys/_intsup.h>

#include <array>
#include <cstdint>
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
static const char* kTag = "AudioSampler";

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
}  // namespace

// --- Static Factory Method ---
std::unique_ptr<AudioSampler> AudioSampler::Create() {
    ESP_LOGI(kTag, "Attempting to create and initialize AudioSampler...");

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
        i2s_del_channel(
            rx_handle);  // CRITICAL: Clean up partially acquired resource
        return nullptr;
    }

    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to enable I2S channel: %s",
                 esp_err_to_name(err));
        i2s_del_channel(
            rx_handle);  // CRITICAL: Clean up partially acquired resource
        return nullptr;
    }

    ESP_LOGI(kTag, "AudioSampler created successfully.");

    // Use `new` because constructor is private. std::make_unique cannot access it.
    return std::unique_ptr<AudioSampler>(new AudioSampler(rx_handle));
}

// --- Private Constructor Implementation ---
AudioSampler::AudioSampler(i2s_chan_handle_t handle) : rx_handle_(handle) {
    ESP_LOGI(kTag, "AudioSampler instance constructed.");
}

// --- Destructor ---
AudioSampler::~AudioSampler() {
    if (rx_handle_ != nullptr) {
        ESP_LOGI(kTag, "Disabling I2S channel...");
        ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
    }
    ESP_LOGI(kTag, "AudioSampler destructed.");
}

// --- Move Constructor ---
AudioSampler::AudioSampler(AudioSampler&& other) noexcept
    : rx_handle_(other.rx_handle_) {
    ESP_LOGI(kTag, "AudioSampler move constructed.");
    other.rx_handle_ = nullptr;
}

// --- Move Assignment Operator ---
AudioSampler& AudioSampler::operator=(AudioSampler&& other) noexcept {
    if (this != &other) {
        if (rx_handle_ != nullptr) {
            i2s_channel_disable(rx_handle_);
        }

        // Transfer ownership of the I2S handle
        rx_handle_ = other.rx_handle_;
        other.rx_handle_ = nullptr;
    }
    ESP_LOGI(kTag, "AudioSampler move assigned.");
    return *this;
}

// --- Read Method ---
esp_err_t AudioSampler::Read(int16_t* dest, size_t samples,
                             size_t* samples_read) {
    if (!rx_handle_) {
        ESP_LOGE(kTag, "I2S channel is not initialized.");
        return ESP_FAIL;
    }

    // Validate samples argument
    if (samples > kMaxAudioSamples) {
        ESP_LOGE(
            kTag,
            "Requested sample count (%zu) exceeds maximum buffer size (%zu).",
            samples, kMaxAudioSamples);
        return ESP_ERR_INVALID_ARG;
    }
    if (samples == 0) {
        *samples_read = 0;
        return ESP_OK;
    }

    // Read as 32-bit samples internally
    std::array<int32_t, kMaxAudioSamples> bit32_buffer;
    size_t bytes_read = 0;

    esp_err_t ret =
        i2s_channel_read(rx_handle_, bit32_buffer.data(),
                         samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "I2S read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    *samples_read = bytes_read / sizeof(int32_t);

    // Convert 32-bit samples to 16-bit samples with clipping
    for (size_t i = 0; i < *samples_read; i++) {
        int32_t value =
            bit32_buffer[i] >>
            kAdcToPcmBitShift;  // Right shift 12 bits, 32-bit → 20-bit valid data
        dest[i] = (value > INT16_MAX)    ? INT16_MAX
                  : (value < -INT16_MAX) ? -INT16_MAX
                                         : (int16_t)value;
    }

    return ESP_OK;
}
