#ifndef APP_STATES_STATE_BASE_HPP_
#define APP_STATES_STATE_BASE_HPP_

namespace app {

class Application;

/**
 * @enum AppState
 * @brief Defines all possible states for the Application state machine.
 */
enum class AppState {
    kUninitialized,
    kInitializing,
    kWaitingForConnection,
    kConnectedIdle,
    kStreamingAudio,
    kFatalError,
};

/**
 * @class StateBase
 * @brief An abstract base class that defines the common interface for all
 * states in the Application's state machine.
 *
 * Each concrete state will inherit from this class and implement its pure
 * virtual functions. The state machine's context (the Application class) will
 * hold a pointer to an instance of a StateBase derivative.
 */
class StateBase {
   public:
    /**
   * @brief Constructs a new StateBase object.
   * @param context A reference to the Application context, allowing states to
   * access shared resources and trigger transitions.
   */
    explicit StateBase(Application& context);

    /**
   * @brief Virtual destructor.
   *
   * Essential for a base class with virtual functions to ensure proper cleanup
   * of derived class instances.
   */
    virtual ~StateBase() = default;

    // Deleting copy and move semantics for state objects as they are unique and
    // managed by the Application context.
    StateBase(const StateBase&) = delete;
    StateBase& operator=(const StateBase&) = delete;
    StateBase(StateBase&&) = delete;
    StateBase& operator=(StateBase&&) = delete;

    /**
   * @brief Called once when the state machine transitions into this state.
   *
   * This method can be overridden by derived states to perform setup actions,
   * such as starting a timer or enabling a peripheral.
   */
    virtual void OnEnter() {}

    /**
   * @brief Called once when the state machine transitions out of this state.
   *
   * This method can be overridden by derived states to perform cleanup
   * actions, such as stopping a timer or disabling a peripheral.
   */
    virtual void OnExit() {}

    /**
   * @brief The main execution logic for the state.
   *
   * This pure virtual function must be implemented by all derived states. It
   * is called repeatedly by the Application's main task while this state is
   * active.
   */
    virtual void Execute() = 0;

    /**
   * @brief Returns the enum identifier corresponding to this state.
   * @return The AppState enum value for this concrete state.
   */
    virtual AppState GetStateEnum() const = 0;

   protected:
    // A reference to the Application context, providing derived states with a
    // way to interact with the rest of the system (e.g., to access the
    // BLEManager or trigger a state transition via Application::SetState).
    Application& context_;
};

}  // namespace app

#endif  // APP_STATES_STATE_BASE_HPP_