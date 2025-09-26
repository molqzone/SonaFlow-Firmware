#ifndef BLE_MANAGER_HPP_
#define BLE_MANAGER_HPP_

#include <array>
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
     * @brief Singleton Factory Method. Creates and initializes the unique BLEManager instance.
     *
     * This is the only way to create the BLEManager object. It ensures the instance is
     * properly initialized and handles potential initialization failures by returning
     * an error code.
     *
     * @return esp_err_t Returns ESP_OK on success, or an error code if initialization fails.
     */
    static esp_err_t CreateInstance();

    /**
     * @brief Gets the singleton instance of the BLEManager.
     * @return A pointer to the unique BLEManager instance.
     */
    static BLEManager* GetInstance();

    ~BLEManager() = default;

    // Deleting copy constructor and assignment operator to enforce singleton.
    BLEManager(const BLEManager&) = delete;
    BLEManager& operator=(const BLEManager&) = delete;

    static int BLEGapEventHandler(struct ble_gap_event* event, void* arg);

    // Handles stack synchronization events.
    static void BLESyncEventHandler(void);

    // Handles stack reset events.
    static void BLEResetEventHandler(int reason);

    /**
     * @brief Starts BLE advertising.
     * * Configures and starts the BLE advertisement process. This method
     * makes the device discoverable by other BLE central devices.
     */
    esp_err_t StartAdvertising();

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
     * @brief Static callback for GATT (Generic Attribute Profile) access events.
     *
     * This C-style callback handles read/write requests to our characteristics.
     */
    static int GattAccessCallback(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt* ctxt, void* arg);

   private:
    BLEManager() = default;

    /**
     * @brief Factory method for creating the unique_ptr instance.
     * This is called internally by CreateInstance().
     */
    static std::unique_ptr<BLEManager> Create();

    /**
     * @brief Internal initialization method called by CreateInstance.
     *
     * Sets up NVS, the NimBLE port, host configurations, services, and starts
     * the necessary FreeRTOS tasks.
     *
     * @return ESP_OK on success, or an ESP-IDF error code on failure.
     */
    esp_err_t Initialize();

    /**
     * @brief The main task for the NimBLE host stack.
     *
     * This task runs the nimble_port_run() event loop, which processes all
     * incoming BLE events. This function must not return.
     */
    static void NimbleHostTask(void* param);

    /**
     * @brief FreeRTOS task that sends outgoing BLE packets from the queue.
     *
     * This task waits for encoded data arrays to be placed in the send queue.
     * Upon receiving an item, it sends it as a BLE notification to the connected
     * client.
     *
     * @param param A void pointer to the BLEManager instance that owns this task.
     */
    static void SendTask(void* param);

    /**
     * @brief Static callback for NimBLE stack synchronization events.
     */
    static void OnSync();

    /**
     * @brief Static callback for NimBLE stack reset events.
     */
    static void OnReset(int reason);

    /**
     * @brief Static callback for all GAP (Generic Access Profile) events.
     *
     * This is the C-style entry point for connection, disconnection, etc.
     * It forwards the event to the singleton's non-static handler.
     */
    static int GapEventHandler(struct ble_gap_event* event, void* arg);

    /**
     * @brief Non-static handler for GAP events.
     *
     * Processes events like connect and disconnect, and invokes user callbacks.
     */
    void HandleGapEvent(struct ble_gap_event* event);

    /**
     * @brief Defines and registers all GATT services and characteristics.
     */
    esp_err_t RegisterGattServices();

    static std::unique_ptr<BLEManager> s_instance_;

    std::function<void()> on_connected_cb_;
    std::function<void()> on_disconnected_cb_;
    std::function<void(const ble::AudioPacket&)> on_audio_packet_received_cb_;
    std::function<void(const std::string& error_message)> on_error_cb_;

    uint16_t conn_handle_ = BLE_HS_CONN_HANDLE_NONE;

    QueueHandle_t send_queue_ = nullptr;
    TaskHandle_t send_task_handle_ = nullptr;
};

}  // namespace ble

#endif  // BLE_MANAGER_HPP_
