#ifndef APP_STATES_STREAMING_STATE_HPP_
#define APP_STATES_STREAMING_STATE_HPP_

#include <cstdint>
#include "states/state_base.hpp"

namespace app {

/**
 * @class StreamingState
 * @brief Represents the state where the device is actively sampling audio,
 * processing features, and transmitting them over a BLE connection.
 */
class StreamingState : public StateBase {
   public:
    explicit StreamingState(Application& context);

    void OnEnter() override;
    void Execute() override;
    AppState GetStateEnum() const override;

   private:
    // A sequence number for the BLE packets, local to this state.
    // It is reset every time a new streaming session starts (in OnEnter).
    uint16_t sequence_number_ = 0;
};

}  // namespace app

#endif  // APP_STATES_STREAMING_STATE_HPP_
