#ifndef ENGINE_STATE_MACHINE_H
#define ENGINE_STATE_MACHINE_H

#include <string>

enum class EngineState {
    Uninitialized,
    Bootstrapping,
    Running,
    Paused,
    ShuttingDown
};

class EngineStateMachine {
public:
    EngineStateMachine();

    EngineState CurrentState() const;
    bool Is(EngineState state) const;
    bool CanTransitionTo(EngineState state) const;
    bool TransitionTo(EngineState state);
    bool TogglePause();

private:
    EngineState state_;
};

#endif // ENGINE_STATE_MACHINE_H
