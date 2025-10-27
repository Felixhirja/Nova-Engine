#pragma once

#include <string>

#include "CameraFollow.h"

namespace CameraConfigLoader {

// Loads the camera follow configuration from an INI-style profile file.
// Returns true when a profile is found and applied; on failure outConfig is left untouched.
bool LoadCameraFollowConfigProfile(const std::string& path,
                                   const std::string& profileName,
                                   CameraFollow::CameraFollowConfig& outConfig);

// Convenience overload that loads the "default" profile.
bool LoadCameraFollowConfig(const std::string& path,
                            CameraFollow::CameraFollowConfig& outConfig);

} // namespace CameraConfigLoader
