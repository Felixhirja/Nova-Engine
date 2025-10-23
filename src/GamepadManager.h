#pragma once

#include <mutex>
#include <string>

class GamepadManager {
public:
    static GamepadManager& Instance();

    // Lazily ensure that XInput has been probed. Returns true if a usable
    // XInput DLL was located and the entry points were resolved.
    bool EnsureInitialized();

    // Release any loaded libraries and reset the state so initialization can
    // be attempted again.
    void Shutdown();

    // Returns true if a working XInput library has been located in the current
    // process.
    bool IsXInputAvailable() const;

    // Returns the UTF-8 encoded name of the loaded XInput DLL. Empty when no
    // library is available.
    std::string ActiveLibraryNameUtf8() const;

    // Human readable description of the most recent initialization error.
    std::string LastError() const;

    // Indicates whether EnsureInitialized() has already attempted to probe
    // XInput in this process.
    bool HasAttemptedInitialization() const;

private:
    GamepadManager();
    ~GamepadManager();

    GamepadManager(const GamepadManager&) = delete;
    GamepadManager& operator=(const GamepadManager&) = delete;

    void ResetState();
    void InitializeInternal();

#if defined(_WIN32)
    struct XInputFunctions {
        void* getState;
        void* getCapabilities;

        XInputFunctions() : getState(nullptr), getCapabilities(nullptr) {}
    };
#endif

#if defined(_WIN32)
    void* xinputModule_;
    XInputFunctions xinputFunctions_;
#endif

    bool attemptedInit_;
    bool xinputAvailable_;
    std::wstring activeLibrary_;
    std::string lastError_;

    mutable std::mutex mutex_;
};

