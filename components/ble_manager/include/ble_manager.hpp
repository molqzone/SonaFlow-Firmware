#ifndef BLE_MANAGER_HPP_
#define BLE_MANAGER_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// NimBLE header files
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Your custom packet header
#include "ble_packet.hpp"

namespace ble {

/**
 * @class BLEManager
 * @brief A singleton class to manage the BLE Host on ESP32.
 * * BLEManager encapsulates the complexities of the NimBLE stack, providing
 * a simple, thread-safe interface for a BLE peripheral device. It handles
 * advertising, connection events, data transfer, and security.
 * The class is implemented as a singleton to ensure a single, consistent
 * point of control over the BLE radio.
 */
class BLEManager {
   public:
    /**
     * @brief Gets the singleton instance of the BLEManager.
     * @return A pointer to the unique BLEManager instance.
     */
    static BLEManager* GetInstance();

    // Deleting copy constructor and assignment operator to enforce singleton.
    BLEManager(const BLEManager&) = delete;
    BLEManager& operator=(const BLEManager&) = delete;

    /**
     * @brief Starts BLE advertising.
     * * Configures and starts the BLE advertisement process. This method
     * makes the device discoverable by other BLE central devices.
     */
    void StartAdvertising();

    /**
     * @brief Stops BLE advertising.
     * * Halts the current BLE advertisement process. The device will no
     * longer be discoverable.
     */
    void StopAdvertising();

    /**
     * @brief Sends an audio data packet over BLE.
     * * Queues an audio packet for transmission. The actual sending is
     * handled by a dedicated FreeRTOS task to avoid blocking the
     * calling thread.
     * * @param packet The ble::AudioPacket to be sent.
     * @return esp_err_t ESP_OK on success, or an error code otherwise.
     */
    esp_err_t SendAudioPacket(const ble::AudioPacket& packet);

    /**
   * @brief Disconnects the current BLE connection.
   * * Initiates a disconnection from the currently connected central device.
   * If not connected, this method does nothing.
   */
    void Disconnect();

    /**
     * @brief Sets the callback for a successful connection event.
     * @param callback The function to be called when a connection is established.
     */
    void SetOnConnectedCallback(std::function<void()> callback);

    /**
     * @brief Sets the callback for a disconnection event.
     * @param callback The function to be called when the device disconnects.
     */
    void SetOnDisconnectedCallback(std::function<void()> callback);

    /**
     * @brief Sets the callback for receiving an audio packet.
     * @param callback The function to be called with the received audio packet.
     */
    void SetOnAudioPacketReceivedCallback(
        std::function<void(const ble::AudioPacket&)> callback);

    /**
     * @brief Sets the callback for reporting errors.
     * @param callback The function to be called with an error message.
     */
    void SetOnErrorCallback(
        std::function<void(const std::string& error_message)> callback);

    /**
     * @brief Configures the BLE connection parameters.
     * @param min_interval The minimum connection interval (units of 1.25 ms).
     * @param max_interval The maximum connection interval (units of 1.25 ms).
     * @param latency The slave latency.
     * @param timeout The connection supervision timeout (units of 10 ms).
     */
    void ConfigureConnectionParams(uint16_t min_interval, uint16_t max_interval,
                                   uint16_t latency, uint16_t timeout);

    /**
     * @brief Sets the maximum transmission unit (MTU) size.
     * @param mtu The desired MTU value.
     */
    void SetMaxMtu(uint16_t mtu);

    /**
     * @brief Enables BLE security features.
     * @param require_mitm If true, requires Man-in-the-Middle protection.
     * @param io_cap The I/O capabilities of the device (e.g., display, keyboard).
     */
    void EnableSecurity(bool require_mitm = true);

   private:
    BLEManager();
    ~BLEManager() = default;

    // NimBLE uses a single unified event handler for all host-level events.
    static int BleHostEventHandler(struct ble_hs_event* event, void* arg);

    // Instead of a single GATT server handler, NimBLE uses callbacks for each characteristic.
    // The implementations will be in the .cpp file.
    static int OnAudioCharacteristicWrite(uint16_t conn_handle,
                                          uint16_t attr_handle,
                                          struct ble_gatt_access_ctxt* ctxt,
                                          void* arg);
    static int OnAudioCharacteristicRead(uint16_t conn_handle,
                                         uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt* ctxt,
                                         void* arg);

    /**
     * @brief Internal method to process received raw data.
     * @param data Pointer to the raw data buffer.
     * @param length The size of the data buffer.
     */
    void HandleReceivedData(const uint8_t* data, size_t length);

    /**
     * @brief Internal method to send an encoded packet.
     * @param packet_data The byte vector of the encoded packet.
     * @return True if the packet was successfully queued for sending, false otherwise.
     */
    bool InternalSendPacket(const std::vector<uint8_t>& packet_data);

    /**
     * @brief FreeRTOS task function for handling the send queue.
     * @param arg Task arguments (unused in this case).
     */
    static void SendTask(void* arg);

    // Private member variables to hold the state of the BLE manager.
    EventGroupHandle_t connection_events_ = nullptr;

    std::function<void()> on_connected_cb_;
    std::function<void()> on_disconnected_cb_;
    std::function<void(const ble::AudioPacket&)> on_audio_packet_received_cb_;
    std::function<void(const std::string& error_message)> on_error_cb_;

    uint16_t conn_id_ = 0;

    QueueHandle_t send_queue_ = nullptr;
    TaskHandle_t send_task_handle_ = nullptr;
};

}  // namespace ble

#endif  // BLE_MANAGER_HPP_