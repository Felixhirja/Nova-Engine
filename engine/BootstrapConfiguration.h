#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Simple configuration container for engine bootstrap settings.  The goal is to
// externalize which high-level frameworks (input, audio, rendering) should be
// initialized for a given run so that integration harnesses and experimental
// frontends can toggle them without recompiling the engine.
struct BootstrapConfiguration {
    bool loadInput = true;
    bool loadAudio = true;
    bool loadRendering = true;
    std::vector<std::string> optionalFrameworks;  // Additional framework identifiers

    static BootstrapConfiguration LoadFromFile(const std::filesystem::path& path,
                                               std::vector<std::string>* warnings = nullptr);
};

