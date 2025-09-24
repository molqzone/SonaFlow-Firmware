#ifndef BLE_PROTOCOL_HPP_
#define BLE_PROTOCOL_HPP_

#include <memory>
class BleProtocol {
   public:
    BleProtocol() {}
    ~BleProtocol() {}

    bool Init();
    bool StartAudioStream();
    bool StopAudioStream();
};

#endif  // BLE_PROTOCOL_HPP_
