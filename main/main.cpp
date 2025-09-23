#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "audio_sampler.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Constants ---
static const char* TAG = "MAIN";

void audio_reader_task(void* param) {
    AudioSampler* sampler = static_cast<AudioSampler*>(param);

    // Use a C++ vector for automatic memory management.
    // This is safer and avoids manual malloc/free,
    // preventing the memory leak in the original code.
    const size_t buffer_samples = 256;
    std::vector<int16_t> samples(buffer_samples);
    const size_t buffer_bytes = samples.size() * sizeof(int16_t);

    ESP_LOGI(TAG, "Audio reader task started. Reading %d bytes per block.",
             buffer_bytes);

    while (true) {
        size_t bytes_read = 0;

        // The 'read' function now takes a pointer to the vector's underlying data.
        esp_err_t ret =
            sampler->read(samples.data(), buffer_bytes, &bytes_read);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
            // A delay here prevents the task from monopolizing the CPU on error.
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (bytes_read > 0) {
            // Instead of printing raw values, which can be difficult to read
            // and slow, we dump the raw data as hexadecimal bytes.
            // This is much more robust for debugging I2S alignment issues.
            ESP_LOG_BUFFER_HEXDUMP(TAG, samples.data(), bytes_read,
                                   ESP_LOG_INFO);
        } else {
            // If no data was read, yield to other tasks to prevent a crash.
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    // The vector automatically frees memory when the task function returns.
    // However, this line is still unreachable in a while(true) loop.
    // It's still good practice, but not necessary here.
    vTaskDelete(NULL);
}

// --- Main Entry Point ---
extern "C" void app_main() {
    ESP_LOGI("MAIN", "Starting audio application");

    static AudioSampler sampler;

    // Initialize the sampler and check for errors.
    if (sampler.init() != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize sampler");
        return;
    }

    // Create the audio reader task.
    xTaskCreate(
        [](void* param) {
            AudioSampler* sampler = static_cast<AudioSampler*>(param);
            const size_t buffer_size = 256;
            int16_t buffer[buffer_size];

            while (true) {
                size_t samples_read = 0;
                if (sampler->read(buffer, buffer_size, &samples_read) ==
                        ESP_OK &&
                    samples_read > 0) {
                    for (size_t i = 0; i < samples_read; i++) {
                        printf("%d\n", buffer[i]);
                    }
                }
                vTaskDelay(1);
            }
        },
        "audio_reader", 4096, &sampler, 5, nullptr);

    // Main loop
    while (true) {
        vTaskDelay(1000);
    }
}
