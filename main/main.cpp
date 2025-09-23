#include <stdio.h>

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

    const size_t buffer_samples = 256;
    int16_t* samples = (int16_t*)malloc(buffer_samples * sizeof(int16_t));

    if (!samples) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Audio reader task started");

    while (true) {
        size_t samples_read = 0;

        esp_err_t ret = sampler->read(samples, buffer_samples, &samples_read);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Read error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (samples_read > 0) {
            for (size_t i = 0; i < samples_read; i++) {
                printf("%d\n", samples[i]);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    free(samples);
    vTaskDelete(NULL);
}

// --- Main Entry Point ---
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Audio processing application started");

    static AudioSampler sampler;

    esp_err_t ret = sampler.init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio sampler: %d, %s", ret,
                 esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "AudioSampler initialized successfully.");

    xTaskCreate(audio_reader_task, "audio_reader_task",
                12288,  // Increased stack size for stability
                &sampler, 5, NULL);

    ESP_LOGI(TAG, "Audio reader task created.");

    // Add an infinite loop to prevent app_main from returning
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}