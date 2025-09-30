#include "ble_packet.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace ble {

std::array<uint8_t, PacketConfig::kPacketSize> PacketEncoder::Encode(
    const AudioPacket& packet) {
    std::array<uint8_t, PacketConfig::kPacketSize> encoded_data;

    encoded_data[0] = packet.header;
    encoded_data[1] = packet.data_type;

    // Convert sequence number to big-endian
    encoded_data[2] = (packet.sequence >> 8) & 0xFF;
    encoded_data[3] = packet.sequence & 0xFF;

    // Convert timestamp to big-endian
    encoded_data[4] = (packet.timestamp >> 24) & 0xFF;
    encoded_data[5] = (packet.timestamp >> 16) & 0xFF;
    encoded_data[6] = (packet.timestamp >> 8) & 0xFF;
    encoded_data[7] = packet.timestamp & 0xFF;

    encoded_data[8] = static_cast<uint8_t>(packet.payload);

    // Calculate and set checksum (excluding the checksum byte itself)
    encoded_data[9] =
        CalculateChecksum(encoded_data.data(), PacketConfig::kPacketSize - 1);

    return encoded_data;
}

uint8_t PacketEncoder::CalculateChecksum(const uint8_t* data, size_t size) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

bool PacketDecoder::Decode(const uint8_t* data, size_t size,
                           AudioPacket& packet) {
    if (!ValidatePacket(data, size)) {
        return false;
    }

    packet.header = data[0];
    packet.data_type = data[1];

    // Convert sequence number from big-endian
    packet.sequence = (static_cast<uint16_t>(data[2]) << 8) | data[3];

    // Convert timestamp from big-endian
    packet.timestamp = (static_cast<uint32_t>(data[4]) << 24) |
                       (static_cast<uint32_t>(data[5]) << 16) |
                       (static_cast<uint32_t>(data[6]) << 8) | data[7];

    packet.payload = static_cast<int8_t>(data[8]);
    packet.checksum = data[9];

    return true;
}

bool PacketDecoder::ValidatePacket(const uint8_t* data, size_t size) {
    if (size != PacketConfig::kPacketSize) {
        return false;
    }

    // Check header sync byte
    if (data[0] != PacketConfig::kHeaderSync) {
        return false;
    }

    // Validate checksum (last byte is checksum of preceding bytes)
    uint8_t calculated_checksum =
        PacketEncoder::CalculateChecksum(data, PacketConfig::kPacketSize - 1);

    return calculated_checksum == data[PacketConfig::kPacketSize - 1];
}

}  // namespace ble