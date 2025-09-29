#ifndef APP_STATES_WAITING_STATE_HPP_
#define APP_STATES_WAITING_STATE_HPP_

#include "states/state_base.hpp"

namespace app {

/**
 * @class WaitingForConnectionState
 * @brief Represents the state where the device is advertising and waiting for a
 * BLE client to connect.
 */
class WaitingForConnectionState : public StateBase {
   public:
    explicit WaitingForConnectionState(Application& context);

    void OnEnter() override;
    void Execute() override;
    AppState GetStateEnum() const override;
};

}  // namespace app

#endif  // APP_STATES_WAITING_STATE_HPP_
