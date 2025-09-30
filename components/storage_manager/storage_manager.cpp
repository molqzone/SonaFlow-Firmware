#include "storage_manager.hpp"
#include "ble_packet.hpp"

#include <cassert>

#include "esp_log.h"
#include "esp_spiffs.h"

namespace {
// File-local constants for the StorageManager implementation.
static const char* kTag = "StorageManager";

// The partition label used for SPIFFS. This MUST match the name in your
// partitions.csv file.
static const char* kSpiffsPartitionLabel = "storage";

// The virtual file path where the log file will be stored.
static const char* kLogFilePath = "/spiffs/audio_log.csv";
}  // namespace

namespace storage {

// Definition and initialization of the static singleton instance.
std::unique_ptr<StorageManager> StorageManager::s_instance_ = nullptr;

esp_err_t StorageManager::CreateInstance() {
    if (s_instance_ != nullptr) {
        ESP_LOGW(kTag, "StorageManager instance already created.");
        return ESP_OK;
    }
    s_instance_ = std::unique_ptr<StorageManager>(new StorageManager());
    esp_err_t err = s_instance_->Initialize();
    if (err != ESP_OK) {
        s_instance_.reset();
    }
    return err;
}

StorageManager& StorageManager::GetInstance() {
    assert(s_instance_ != nullptr);
    return *s_instance_;
}

StorageManager::~StorageManager() {
    if (log_file_ != nullptr) {
        ESP_LOGI(kTag, "Closing log file.");
        fclose(log_file_);
    }
    ESP_LOGI(kTag, "Unregistering SPIFFS filesystem.");
    esp_vfs_spiffs_unregister(kSpiffsPartitionLabel);
}

esp_err_t StorageManager::Initialize() {
    ESP_LOGI(kTag, "Initializing and mounting SPIFFS filesystem...");

    esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                  .partition_label = kSpiffsPartitionLabel,
                                  .max_files = 5,
                                  // Format the partition if mounting fails.
                                  .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(kTag, "Failed to mount or format filesystem.");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(
                kTag,
                "Failed to find SPIFFS partition '%s'. Check partitions.csv.",
                kSpiffsPartitionLabel);
        } else {
            ESP_LOGE(kTag, "Failed to initialize SPIFFS (%s)",
                     esp_err_to_name(ret));
        }
        return ret;
    }

    log_file_ = fopen(kLogFilePath, "a");  // Append mode
    if (log_file_ == nullptr) {
        ESP_LOGE(kTag, "Failed to open log file '%s' for writing.",
                 kLogFilePath);
        // Unregister the filesystem on failure.
        esp_vfs_spiffs_unregister(kSpiffsPartitionLabel);
        return ESP_FAIL;
    }

    ESP_LOGI(kTag, "SPIFFS mounted and log file opened successfully.");
    return ESP_OK;
}

esp_err_t StorageManager::LogAudioFeature(const ble::AudioPacket& packet) {
    if (log_file_ == nullptr) {
        ESP_LOGE(kTag, "Log file is not open, cannot log feature.");
        return ESP_ERR_INVALID_STATE;
    }

    // Write the data in a simple CSV format for easy parsing later.
    // Format: timestamp,sequence,payload
    fprintf(log_file_, "%u,%u,%d\n", packet.timestamp, packet.sequence,
            packet.payload);

    // Flushing the file buffer after every write ensures data is immediately
    // written to flash, which is safer against sudden power loss. For very
    // high-frequency logging, this could be done periodically instead.
    fflush(log_file_);

    return ESP_OK;
}

}  // namespace storage