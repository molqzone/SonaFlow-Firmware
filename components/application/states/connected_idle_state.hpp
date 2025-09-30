#ifndef APP_STATES_CONNECTED_IDLE_STATE_HPP_
#define APP_STATES_CONNECTED_IDLE_STATE_HPP_

#include "states/state_base.hpp"

namespace app {

/**
 * @class ConnectedIdleState
 * @brief Represents the state where a BLE client is connected, but no data
 * streaming is active. The system is idle and waiting for further commands or
 * events.
 */
class ConnectedIdleState : public StateBase {
   public:
    explicit ConnectedIdleState(Application& context);

    void OnEnter() override;
    void Execute() override;
    AppState GetStateEnum() const override;
};

}  // namespace app

#endif  // APP_STATES_CONNECTED_IDLE_STATE_HPP_
