#ifndef APP_STATES_FATAL_ERROR_STATE_HPP_
#define APP_STATES_FATAL_ERROR_STATE_HPP_

#include "states/state_base.hpp"

namespace app {

class FatalErrorState : public StateBase {
   public:
    explicit FatalErrorState(Application& context);

    void OnEnter() override;
    void Execute() override;
    AppState GetStateEnum() const override;

   private:
    bool led_on_ = false;
};

}  // namespace app

#endif  // APP_STATES_FATAL_ERROR_STATE_HPP_
