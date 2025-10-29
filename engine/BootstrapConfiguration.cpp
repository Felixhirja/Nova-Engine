#include "BootstrapConfiguration.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace {
std::string Trim(const std::string& input) {
    size_t start = 0;
    size_t end = input.size();
    while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

bool ParseBooleanField(const std::string& source,
                       const std::string& key,
                       bool currentValue,
                       std::vector<std::string>* warnings) {
    const std::string token = "\"" + key + "\"";
    size_t pos = source.find(token);
    if (pos == std::string::npos) {
        return currentValue;
    }

    pos = source.find(':', pos + token.size());
    if (pos == std::string::npos) {
        if (warnings) {
            warnings->push_back("Bootstrap configuration: missing ':' after key '" + key + "'.");
        }
        return currentValue;
    }

    ++pos;
    while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
        ++pos;
    }

    if (source.compare(pos, 4, "true") == 0) {
        return true;
    }
    if (source.compare(pos, 5, "false") == 0) {
        return false;
    }

    if (warnings) {
        warnings->push_back("Bootstrap configuration: key '" + key + "' is not a boolean; using default.");
    }
    return currentValue;
}

void ParseOptionalFrameworks(const std::string& source,
                             std::vector<std::string>& outFrameworks,
                             std::vector<std::string>* warnings) {
    const std::string key = "\"optionalFrameworks\"";
    size_t pos = source.find(key);
    if (pos == std::string::npos) {
        return;
    }

    size_t start = source.find('[', pos + key.size());
    size_t end = start == std::string::npos ? std::string::npos : source.find(']', start + 1);
    if (start == std::string::npos || end == std::string::npos) {
        if (warnings) {
            warnings->push_back("Bootstrap configuration: optionalFrameworks missing array delimiters; ignoring entry.");
        }
        return;
    }

    std::string arrayContent = source.substr(start + 1, end - start - 1);
    std::stringstream ss(arrayContent);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = Trim(item);
        if (item.size() < 2 || item.front() != '"' || item.back() != '"') {
            if (item.empty()) {
                continue;
            }
            if (warnings) {
                warnings->push_back("Bootstrap configuration: optional framework entry '" + item + "' is not a string literal; skipping.");
            }
            continue;
        }
        outFrameworks.push_back(item.substr(1, item.size() - 2));
    }
}

} // namespace

BootstrapConfiguration BootstrapConfiguration::LoadFromFile(const std::filesystem::path& path,
                                                             std::vector<std::string>* warnings) {
    BootstrapConfiguration config;

    std::ifstream file(path);
    if (!file.is_open()) {
        if (warnings) {
            warnings->push_back("Bootstrap configuration not found at '" + path.string() + "'; using defaults.");
        }
        return config;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();

    config.loadInput = ParseBooleanField(contents, "input", config.loadInput, warnings);
    config.loadAudio = ParseBooleanField(contents, "audio", config.loadAudio, warnings);
    config.loadRendering = ParseBooleanField(contents, "rendering", config.loadRendering, warnings);

    ParseOptionalFrameworks(contents, config.optionalFrameworks, warnings);

    return config;
}

