#include "EngineStateMachine.h"

#include <iostream>

EngineStateMachine::EngineStateMachine() : state_(EngineState::Uninitialized) {}

EngineState EngineStateMachine::CurrentState() const {
    return state_;
}

bool EngineStateMachine::Is(EngineState state) const {
    return state_ == state;
}

bool EngineStateMachine::CanTransitionTo(EngineState state) const {
    if (state_ == state) {
        return true;
    }

    switch (state_) {
        case EngineState::Uninitialized:
            return state == EngineState::Bootstrapping || state == EngineState::ShuttingDown;
        case EngineState::Bootstrapping:
            return state == EngineState::Running || state == EngineState::ShuttingDown;
        case EngineState::Running:
            return state == EngineState::Paused || state == EngineState::ShuttingDown;
        case EngineState::Paused:
            return state == EngineState::Running || state == EngineState::ShuttingDown;
        case EngineState::ShuttingDown:
            return false;
    }

    return false;
}

bool EngineStateMachine::TransitionTo(EngineState state) {
    if (!CanTransitionTo(state)) {
        std::cerr << "Invalid engine state transition from " << static_cast<int>(state_)
                  << " to " << static_cast<int>(state) << std::endl;
        return false;
    }

    state_ = state;
    return true;
}

bool EngineStateMachine::TogglePause() {
    if (state_ == EngineState::Running) {
        return TransitionTo(EngineState::Paused);
    }
    if (state_ == EngineState::Paused) {
        return TransitionTo(EngineState::Running);
    }
    std::cerr << "Cannot toggle pause from state " << static_cast<int>(state_) << std::endl;
    return false;
}
