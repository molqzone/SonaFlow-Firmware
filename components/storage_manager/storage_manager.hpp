#ifndef APP_STORAGE_MANAGER_HPP_
#define APP_STORAGE_MANAGER_HPP_

#include <memory>
#include <string>
#include "ble_packet.hpp"
#include "esp_err.h"

namespace storage {

/**
 * @class StorageManager
 * @brief Manages file storage on the device's internal SPI flash using SPIFFS.
 */
class StorageManager {
   public:
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;
    ~StorageManager();

    /**
     * @brief Creates and initializes the unique StorageManager instance.
     * This will mount the SPIFFS filesystem.
     * @return esp_err_t ESP_OK on success.
     */
    static esp_err_t CreateInstance();

    /**
     * @brief Gets the singleton instance of the StorageManager.
     * @return A reference to the unique StorageManager instance.
     */
    static StorageManager& GetInstance();

    /**
     * @brief Logs an audio feature data point to the storage.
     * @param packet The audio packet containing the feature to be logged.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t LogAudioFeature(const ble::AudioPacket& packet);

   private:
    StorageManager() = default;
    esp_err_t Initialize();

    FILE* log_file_ = nullptr;
    static std::unique_ptr<StorageManager> s_instance_;
};

}  // namespace storage

#endif  // APP_STORAGE_MANAGER_HPP_