#include "CameraConfigLoader.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::string Trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

bool ParseDouble(const std::string& text, double& outValue) {
    errno = 0;
    char* endPtr = nullptr;
    const char* begin = text.c_str();
    double value = std::strtod(begin, &endPtr);
    if (begin == endPtr || errno == ERANGE) {
        return false;
    }
    while (endPtr && *endPtr != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*endPtr))) {
            return false;
        }
        ++endPtr;
    }
    outValue = value;
    return true;
}

bool ParseInt(const std::string& text, int& outValue) {
    errno = 0;
    char* endPtr = nullptr;
    const char* begin = text.c_str();
    long value = std::strtol(begin, &endPtr, 10);
    if (begin == endPtr || errno == ERANGE) {
        return false;
    }
    while (endPtr && *endPtr != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*endPtr))) {
            return false;
        }
        ++endPtr;
    }
    outValue = static_cast<int>(value);
    return true;
}

bool ParseBool(const std::string& text, bool& outValue) {
    std::string lower;
    lower.reserve(text.size());
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        outValue = true;
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        outValue = false;
        return true;
    }
    return false;
}

bool ApplyKeyValue(CameraFollow::CameraFollowConfig& config,
                   const std::string& key,
                   const std::string& value) {
    double numeric = 0.0;
    bool boolValue = false;
    int intValue = 0;

    if (key == "orbitDistance" && ParseDouble(value, numeric)) {
        config.orbitDistance = numeric;
    } else if (key == "orbitHeight" && ParseDouble(value, numeric)) {
        config.orbitHeight = numeric;
    } else if (key == "minDistanceFromPlayer" && ParseDouble(value, numeric)) {
        config.minDistanceFromPlayer = numeric;
    } else if (key == "groundLevel" && ParseDouble(value, numeric)) {
        config.groundLevel = numeric;
    } else if (key == "terrainBuffer" && ParseDouble(value, numeric)) {
        config.terrainBuffer = numeric;
    } else if (key == "transitionSpeed" && ParseDouble(value, numeric)) {
        config.transitionSpeed = numeric;
    } else if (key == "posResponsiveness" && ParseDouble(value, numeric)) {
        config.posResponsiveness = numeric;
    } else if (key == "rotResponsiveness" && ParseDouble(value, numeric)) {
        config.rotResponsiveness = numeric;
    } else if (key == "maxDeltaTimeClamp" && ParseDouble(value, numeric)) {
        config.maxDeltaTimeClamp = numeric;
    } else if (key == "moveSpeedHorizontal" && ParseDouble(value, numeric)) {
        config.moveSpeedHorizontal = numeric;
    } else if (key == "moveSpeedVertical" && ParseDouble(value, numeric)) {
        config.moveSpeedVertical = numeric;
    } else if (key == "freeAccelHz" && ParseDouble(value, numeric)) {
        config.freeAccelHz = numeric;
    } else if (key == "sprintMultiplier" && ParseDouble(value, numeric)) {
        config.sprintMultiplier = numeric;
    } else if (key == "pitchAffectsForward" && ParseBool(value, boolValue)) {
        config.pitchAffectsForward = boolValue;
    } else if (key == "freeVelDeadzone" && ParseDouble(value, numeric)) {
        config.freeVelDeadzone = numeric;
    } else if (key == "freeLookSensYaw" && ParseDouble(value, numeric)) {
        config.freeLookSensYaw = numeric;
    } else if (key == "freeLookSensPitch" && ParseDouble(value, numeric)) {
        config.freeLookSensPitch = numeric;
    } else if (key == "invertFreeLookYaw" && ParseBool(value, boolValue)) {
        config.invertFreeLookYaw = boolValue;
    } else if (key == "invertFreeLookPitch" && ParseBool(value, boolValue)) {
        config.invertFreeLookPitch = boolValue;
    } else if (key == "invertLockYaw" && ParseBool(value, boolValue)) {
        config.invertLockYaw = boolValue;
    } else if (key == "invertLockPitch" && ParseBool(value, boolValue)) {
        config.invertLockPitch = boolValue;
    } else if (key == "shoulderOffset" && ParseDouble(value, numeric)) {
        config.shoulderOffset = numeric;
    } else if (key == "dynamicShoulderFactor" && ParseDouble(value, numeric)) {
        config.dynamicShoulderFactor = numeric;
    } else if (key == "pitchBias" && ParseDouble(value, numeric)) {
        config.pitchBias = numeric;
    } else if (key == "pitchMin" && ParseDouble(value, numeric)) {
        config.pitchMin = numeric;
    } else if (key == "pitchMax" && ParseDouble(value, numeric)) {
        config.pitchMax = numeric;
    } else if (key == "topBlendScale" && ParseDouble(value, numeric)) {
        config.topBlendScale = numeric;
    } else if (key == "clampPitch" && ParseBool(value, boolValue)) {
        config.clampPitch = boolValue;
    } else if (key == "alwaysTickFreeMode" && ParseBool(value, boolValue)) {
        config.alwaysTickFreeMode = boolValue;
    } else if (key == "nearVerticalDeg" && ParseDouble(value, numeric)) {
        config.nearVerticalDeg = numeric;
    } else if (key == "softGroundClamp" && ParseBool(value, boolValue)) {
        config.softGroundClamp = boolValue;
    } else if (key == "groundClampHz" && ParseDouble(value, numeric)) {
        config.groundClampHz = numeric;
    } else if (key == "enableObstacleAvoidance" && ParseBool(value, boolValue)) {
        config.enableObstacleAvoidance = boolValue;
    } else if (key == "obstacleMargin" && ParseDouble(value, numeric)) {
        config.obstacleMargin = numeric;
    } else if (key == "enableTeleportHandling" && ParseBool(value, boolValue)) {
        config.enableTeleportHandling = boolValue;
    } else if (key == "teleportDistanceThreshold" && ParseDouble(value, numeric)) {
        config.teleportDistanceThreshold = numeric;
    } else if (key == "teleportSnapFrames" && ParseInt(value, intValue)) {
        config.teleportSnapFrames = intValue;
    } else if (key == "teleportBlendSeconds" && ParseDouble(value, numeric)) {
        config.teleportBlendSeconds = numeric;
    } else if (key == "teleportBlendMinAlpha" && ParseDouble(value, numeric)) {
        config.teleportBlendMinAlpha = numeric;
    } else {
        return false;
    }
    return true;
}

bool ParseCameraConfigStream(std::istream& input,
                             std::unordered_map<std::string, CameraFollow::CameraFollowConfig>& outProfiles) {
    std::string line;
    std::string currentProfile;
    CameraFollow::CameraFollowConfig currentConfig;
    bool inProfile = false;

    auto commit = [&]() {
        if (!currentProfile.empty()) {
            currentConfig.Validate();
            outProfiles[currentProfile] = currentConfig;
        }
    };

    while (std::getline(input, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commit();
            }
            currentProfile = Trim(trimmed.substr(1, trimmed.size() - 2));
            currentConfig = CameraFollow::CameraFollowConfig{};
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

        std::string key = Trim(trimmed.substr(0, equalsPos));
        std::string value = Trim(trimmed.substr(equalsPos + 1));
        if (key.empty()) {
            continue;
        }
        ApplyKeyValue(currentConfig, key, value);
    }

    if (inProfile) {
        commit();
    }

    return !outProfiles.empty();
}

bool IsRelativePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return false;
    }
    if (path.size() > 1 && path[1] == ':') {
        return false;
    }
    return true;
}

bool LoadProfiles(const std::string& path,
                  std::unordered_map<std::string, CameraFollow::CameraFollowConfig>& outProfiles) {
    std::vector<std::string> candidates;
    candidates.push_back(path);
    if (IsRelativePath(path)) {
        candidates.push_back("../" + path);
        candidates.push_back("../../" + path);
    }

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (!file.is_open()) {
            continue;
        }
        std::unordered_map<std::string, CameraFollow::CameraFollowConfig> parsed;
        if (ParseCameraConfigStream(file, parsed)) {
            outProfiles = std::move(parsed);
            return true;
        }
    }
    return false;
}

} // namespace

namespace CameraConfigLoader {

bool LoadCameraFollowConfigProfile(const std::string& path,
                                   const std::string& profileName,
                                   CameraFollow::CameraFollowConfig& outConfig) {
    std::unordered_map<std::string, CameraFollow::CameraFollowConfig> profiles;
    if (!LoadProfiles(path, profiles)) {
        return false;
    }

    auto it = profiles.find(profileName);
    if (it != profiles.end()) {
        outConfig = it->second;
        return true;
    }

    auto defaultIt = profiles.find("default");
    if (defaultIt != profiles.end()) {
        outConfig = defaultIt->second;
        return true;
    }

    if (!profiles.empty()) {
        outConfig = profiles.begin()->second;
        return true;
    }

    return false;
}

bool LoadCameraFollowConfig(const std::string& path,
                            CameraFollow::CameraFollowConfig& outConfig) {
    return LoadCameraFollowConfigProfile(path, "default", outConfig);
}

} // namespace CameraConfigLoader
