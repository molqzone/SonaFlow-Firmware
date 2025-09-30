#ifndef APP_STATES_UNINITIALIZED_STATE_HPP_
#define APP_STATES_UNINITIALIZED_STATE_HPP_

#include "states/state_base.hpp"

namespace app {

class UninitializedState : public StateBase {
   public:
    explicit UninitializedState(Application& context);
    void Execute() override;
    AppState GetStateEnum() const override;
};

}  // namespace app

#endif  // APP_STATES_UNINITIALIZED_STATE_HPP_
