#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "audio_sampler.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Constants ---
static const char* kTag = "MAIN";

void audio_reader_task(void* param) {
    AudioSampler* sampler = static_cast<AudioSampler*>(param);

    // Use a C++ vector for automatic memory management.
    const size_t buffer_samples = 256;
    std::vector<int16_t> samples(buffer_samples);
    const size_t buffer_bytes = samples.size() * sizeof(int16_t);

    ESP_LOGI(kTag, "Audio reader task started. Reading %d bytes per block.",
             buffer_bytes);

    while (true) {
        size_t bytes_read = 0;

        // The 'read' function now takes a pointer to the vector's underlying data.
        esp_err_t ret =
            sampler->Read(samples.data(), buffer_bytes, &bytes_read);

        if (ret != ESP_OK) {
            ESP_LOGE(kTag, "I2S read error: %s", esp_err_to_name(ret));
            // A delay here prevents the task from monopolizing the CPU on error.
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (bytes_read > 0) {
            // Instead of printing raw values, which can be difficult to read
            // and slow, we dump the raw data as hexadecimal bytes.
            // This is much more robust for debugging I2S alignment issues.
            ESP_LOG_BUFFER_HEXDUMP(kTag, samples.data(), bytes_read,
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

// --- Main Application Entry Point ---
extern "C" void app_main() {
    ESP_LOGI(kTag, "Starting audio application...");

    // 1. Create the sampler using the static factory method.
    // The 'sampler' variable now holds a smart pointer.
    // It's either a valid pointer to a fully initialized object, or nullptr.
    std::unique_ptr<AudioSampler> sampler = AudioSampler::Create();

    // 2. Check if the creation was successful.
    if (!sampler) {
        ESP_LOGE(kTag, "Failed to create AudioSampler instance. Halting.");
        // In a real application, you might want to restart or enter a safe mode.
        return;
    }

    // 3. Create the audio reader task, passing the raw pointer.
    // The unique_ptr in app_main continues to own the object. The task
    // just borrows a pointer to use it. This is safe because the sampler
    // object will exist as long as app_main is running.
    xTaskCreate(
        [](void* param) {
            auto* sampler_ptr = static_cast<AudioSampler*>(param);

            constexpr size_t kBufferSize = 1024;
            std::vector<int16_t> buffer(kBufferSize);

            while (true) {
                size_t samples_read = 0;
                if (sampler_ptr->Read(buffer.data(), kBufferSize,
                                      &samples_read) == ESP_OK &&
                    samples_read > 0) {

                    // --- WARNING: Don't print every sample in a real app! ---
                    // Printing every sample is very slow and will cause buffer
                    // overruns. This is just for demonstration.
                    // A better approach is to process the data or calculate metrics.
                    int64_t sum = 0;
                    for (size_t i = 0; i < samples_read; i++) {
                        sum += abs(buffer[i]);
                    }
                    int32_t average_level = sum / samples_read;
                    printf("Avg audio level: %d\n", average_level);
                }
                // A small delay to prevent the task from starving other tasks if
                // no data is available.
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "audio_reader_task",  // Task name
        4096,                 // Stack size
        sampler.get(),        // Pass the raw pointer from the unique_ptr
        5,                    // Task priority
        nullptr               // Task handle (optional)
    );

    // Main loop can be used for other tasks, like monitoring.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
