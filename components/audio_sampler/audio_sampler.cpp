#include "audio_sampler.hpp"

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
static const char* TAG = "AudioSampler";

// I2S peripheral configuration
constexpr i2s_port_t I2S_PORT = I2S_NUM_AUTO;
constexpr uint32_t I2S_SAMPLE_RATE = 44100;  // 44.1 kHz
constexpr i2s_data_bit_width_t I2S_BITS_PER_SAMPLE = I2S_DATA_BIT_WIDTH_24BIT;
constexpr uint32_t DMA_BUFFER_COUNT = 16;
constexpr uint32_t DMA_BUFFER_SAMPLES = 1024;

// GPIO pin configuration
constexpr gpio_num_t I2S_STD_GPIO_WS = GPIO_NUM_4;
constexpr gpio_num_t I2S_STD_GPIO_BCLK = GPIO_NUM_5;
constexpr gpio_num_t I2S_STD_GPIO_DIN = GPIO_NUM_6;
}  // namespace

// --- Constructor ---
AudioSampler::AudioSampler() : rx_handle_(nullptr) {
    ESP_LOGI(TAG, "AudioSampler constructed. (uninitialized)");
}

// --- Init Method ---
esp_err_t AudioSampler::init() {
    ESP_LOGI(TAG, "Initializing AudioSampler with 32-bit configuration...");

    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = 1024;
    chan_cfg.auto_clear_after_cb = true;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));

    i2s_std_config_t std_cfg = {
        .clk_cfg =
            {
                .sample_rate_hz = 16000,
                .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
#endif
                .mclk_multiple = I2S_MCLK_MULTIPLE_256,
                .bclk_div = 0,
            },
        .slot_cfg =
            {.data_bit_width =
                 I2S_DATA_BIT_WIDTH_32BIT,  // Actually using 20-bit data
             .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
             .slot_mode = I2S_SLOT_MODE_MONO,
             .slot_mask = I2S_STD_SLOT_LEFT,
             .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
             .ws_pol = false,
             .bit_shift = true,  // Enable bit shifting
             .left_align = true,
             .big_endian = false,
             .bit_order_lsb = false},
        .gpio_cfg =
            {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = I2S_STD_GPIO_BCLK,
                .ws = I2S_STD_GPIO_WS,
                .dout = I2S_GPIO_UNUSED,
                .din = I2S_STD_GPIO_DIN,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));

    ESP_LOGI(TAG, "32-bit configuration successful.");
    return ESP_OK;
}

// --- Destructor ---
AudioSampler::~AudioSampler() {
    if (rx_handle_ != nullptr) {
        ESP_LOGI(TAG, "Disabling I2S channel...");
        ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
    }
    ESP_LOGI(TAG, "AudioSampler destructed.");
}

// --- Move Constructor ---
AudioSampler::AudioSampler(AudioSampler&& other) noexcept
    : rx_handle_(other.rx_handle_) {
    ESP_LOGI(TAG, "AudioSampler move constructed.");
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
    ESP_LOGI(TAG, "AudioSampler move assigned.");
    return *this;
}

// --- Read Method ---
esp_err_t AudioSampler::read(int16_t* dest, size_t samples,
                             size_t* samples_read) {
    if (!rx_handle_) {
        ESP_LOGE(TAG, "I2S channel is not initialized.");
        return ESP_FAIL;
    }

    // Read as 32-bit samples internally
    std::vector<int32_t> bit32_buffer(samples);
    size_t bytes_read;

    esp_err_t ret =
        i2s_channel_read(rx_handle_, bit32_buffer.data(),
                         samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    *samples_read = bytes_read / sizeof(int32_t);

    // Convert 32-bit samples to 16-bit samples with clipping
    for (size_t i = 0; i < *samples_read; i++) {
        int32_t value = bit32_buffer[i] >>
                        12;  // Right shift 12 bits, 32-bit → 20-bit valid data
        dest[i] = (value > INT16_MAX)    ? INT16_MAX
                  : (value < -INT16_MAX) ? -INT16_MAX
                                         : (int16_t)value;
    }

    return ESP_OK;
}
