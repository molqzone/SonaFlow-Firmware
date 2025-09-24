#pragma once

#include <cstdint>
#include <vector>

namespace ble {

/**
 * @brief Constants for BLE packet configuration.
 */
struct PacketConfig {
    static constexpr size_t kPacketSize = 10;
    static constexpr uint8_t kHeaderSync = 0xAA;
    static constexpr uint8_t kDataTypeAudio = 0x01;
};

/**
 * @brief BLE audio data packet structure with fixed 10-byte size.
 * 
 * Packet format:
 * - Byte 0: Header sync byte (0xAA)
 * - Byte 1: Data type indicator  
 * - Byte 2-3: Sequence number (big-endian)
 * - Byte 4-7: Timestamp (big-endian)
 * - Byte 8: Audio feature payload
 * - Byte 9: XOR checksum
 */
struct AudioPacket {
    uint8_t header;      // Sync byte
    uint8_t data_type;   // Data type identifier
    uint16_t sequence;   // Packet sequence number
    uint32_t timestamp;  // Timestamp in milliseconds
    int8_t payload;      // Audio feature data
    uint8_t checksum;    // Data integrity check
};

/**
 * @brief Encodes AudioPacket to raw byte buffer for BLE transmission.
 */
class PacketEncoder {
   public:
    /**
   * @brief Encodes an AudioPacket into a byte vector.
   * @param packet Source packet to encode.
   * @return Vector containing encoded bytes.
   */
    static std::vector<uint8_t> Encode(const AudioPacket& packet);

    /**
   * @brief Calculates XOR checksum for data integrity.
   * @param data Pointer to the data buffer.
   * @param size Size of the data buffer.
   * @return Calculated checksum value.
   */
    static uint8_t CalculateChecksum(const uint8_t* data, size_t size);
};

/**
 * @brief Decodes raw byte buffer to AudioPacket.
 */
class PacketDecoder {
   public:
    /**
   * @brief Decodes raw bytes into an AudioPacket.
   * @param data Pointer to the raw byte data.
   * @param size Size of the data buffer.
   * @return Decoded AudioPacket if valid, empty vector if invalid.
   */
    static bool Decode(const uint8_t* data, size_t size, AudioPacket& packet);

    /**
   * @brief Validates packet integrity using header and checksum.
   * @param data Pointer to the packet data.
   * @param size Size of the packet data.
   * @return True if packet is valid, false otherwise.
   */
    static bool ValidatePacket(const uint8_t* data, size_t size);
};

}  // namespace ble