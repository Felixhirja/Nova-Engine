#include "GamepadManager.h"

#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <Xinput.h>
#if defined(_MSC_VER)
#include <excpt.h>
#endif
#endif

namespace {

#if defined(_WIN32)
std::string WStringToUtf8(const std::wstring& value) {
    if (value.empty()) return std::string();

    int requiredSize = WideCharToMultiByte(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (requiredSize <= 0) {
        return std::string();
    }

    std::string result(static_cast<size_t>(requiredSize), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), requiredSize, nullptr, nullptr);
    return result;
}

std::string FormatWindowsError(DWORD code) {
    if (code == 0) {
        return "no error";
    }

    LPWSTR buffer = nullptr;
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::string message;
    if (length != 0 && buffer != nullptr) {
        message = WStringToUtf8(std::wstring(buffer, length));
        LocalFree(buffer);
    } else {
        std::ostringstream oss;
        oss << "error code " << code;
        message = oss.str();
    }
    return message;
}

HMODULE SafeLoadLibrary(const wchar_t* libraryName, std::string& errorOut) {
    HMODULE moduleHandle = nullptr;
#if defined(_MSC_VER)
    __try {
        moduleHandle = LoadLibraryW(libraryName);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DWORD exceptionCode = GetExceptionCode();
        std::ostringstream oss;
        oss << "structured exception 0x" << std::hex << exceptionCode;
        errorOut = oss.str();
        return nullptr;
    }
#else
    moduleHandle = LoadLibraryW(libraryName);
#endif

    if (!moduleHandle) {
        DWORD lastError = GetLastError();
        std::ostringstream oss;
        oss << "LoadLibraryW failed: " << FormatWindowsError(lastError);
        errorOut = oss.str();
    }

    return moduleHandle;
}
#else
std::string WStringToUtf8(const std::wstring& value) {
    return std::string(value.begin(), value.end());
}
#endif

} // namespace

GamepadManager& GamepadManager::Instance() {
    static GamepadManager instance;
    return instance;
}

GamepadManager::GamepadManager()
    :
#if defined(_WIN32)
      xinputModule_(nullptr),
      xinputFunctions_(),
#endif
      attemptedInit_(false),
      xinputAvailable_(false),
      activeLibrary_(),
      lastError_() {}

GamepadManager::~GamepadManager() {
    Shutdown();
}

bool GamepadManager::EnsureInitialized() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!attemptedInit_) {
        InitializeInternal();
    }
    return xinputAvailable_;
}

void GamepadManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
#if defined(_WIN32)
    if (xinputModule_) {
        FreeLibrary(static_cast<HMODULE>(xinputModule_));
        xinputModule_ = nullptr;
    }
    xinputFunctions_ = XInputFunctions();
#endif
    ResetState();
}

bool GamepadManager::IsXInputAvailable() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return xinputAvailable_;
}

std::string GamepadManager::ActiveLibraryNameUtf8() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return WStringToUtf8(activeLibrary_);
}

std::string GamepadManager::LastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastError_;
}

bool GamepadManager::HasAttemptedInitialization() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return attemptedInit_;
}

void GamepadManager::ResetState() {
    attemptedInit_ = false;
    xinputAvailable_ = false;
    activeLibrary_.clear();
    lastError_.clear();
}

void GamepadManager::InitializeInternal() {
    attemptedInit_ = true;
    xinputAvailable_ = false;
    activeLibrary_.clear();
    lastError_.clear();

#if defined(_WIN32)
    const std::vector<std::wstring> candidateLibraries = {
        L"XInput1_4.dll",
        L"XInput1_3.dll",
        L"XInput9_1_0.dll"
    };

    std::vector<std::string> failureDetails;
    for (const auto& candidate : candidateLibraries) {
        std::string failureReason;
        HMODULE handle = SafeLoadLibrary(candidate.c_str(), failureReason);
        if (!handle) {
            std::ostringstream oss;
            oss << WStringToUtf8(candidate) << ": " << failureReason;
            failureDetails.push_back(oss.str());
            continue;
        }

        auto getState = reinterpret_cast<DWORD (*)(DWORD, XINPUT_STATE*)>(
            GetProcAddress(handle, "XInputGetState"));
        auto getCapabilities = reinterpret_cast<DWORD (*)(DWORD, DWORD, XINPUT_CAPABILITIES*)>(
            GetProcAddress(handle, "XInputGetCapabilities"));

        if (!getState || !getCapabilities) {
            std::ostringstream oss;
            oss << WStringToUtf8(candidate)
                << ": missing expected entry points ("
                << (getState ? "" : "XInputGetState ")
                << (getCapabilities ? "" : "XInputGetCapabilities")
                << ")";
            failureDetails.push_back(oss.str());
            FreeLibrary(handle);
            continue;
        }

        xinputModule_ = handle;
        xinputFunctions_.getState = reinterpret_cast<void*>(getState);
        xinputFunctions_.getCapabilities = reinterpret_cast<void*>(getCapabilities);
        activeLibrary_ = candidate;
        xinputAvailable_ = true;
        lastError_.clear();
        break;
    }

    if (!xinputAvailable_) {
        std::ostringstream oss;
        oss << "Failed to load an XInput library";
        if (!failureDetails.empty()) {
            oss << ": ";
            for (size_t i = 0; i < failureDetails.size(); ++i) {
                oss << failureDetails[i];
                if (i + 1 < failureDetails.size()) {
                    oss << "; ";
                }
            }
        }
        lastError_ = oss.str();
    }
#else
    lastError_ = "XInput is only supported on Windows.";
    xinputAvailable_ = false;
#endif
}

