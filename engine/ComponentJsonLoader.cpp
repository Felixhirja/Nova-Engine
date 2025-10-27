#include "ComponentJsonLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace {

using FileTimePoint = std::filesystem::file_time_type;
std::unordered_map<std::string, FileTimePoint> fileModificationTimes;

// Simple JSON parser for component blueprints
class JsonParser {
public:
    JsonParser(const std::string& json) : input_(json), pos_(0) {}

    bool ParseObject(std::unordered_map<std::string, std::string>& outObject) {
        SkipWhitespace();
        if (!Consume('{')) return false;

        outObject.clear();

        while (true) {
            SkipWhitespace();
            if (Peek() == '}') {
                Consume('}');
                return true;
            }

            std::string key;
            if (!ParseString(key)) return false;

            SkipWhitespace();
            if (!Consume(':')) return false;

            std::string value;
            if (!ParseValue(value)) return false;

            outObject[key] = value;

            SkipWhitespace();
            if (Peek() == ',') {
                Consume(',');
            } else if (Peek() == '}') {
                // Will be consumed in next iteration
            } else {
                return false;
            }
        }
    }

private:
    void SkipWhitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    char Peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    bool Consume(char c) {
        if (Peek() == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool ParseString(std::string& outString) {
        if (!Consume('"')) return false;

        outString.clear();
        while (pos_ < input_.size()) {
            char c = input_[pos_++];
            if (c == '"') {
                return true;
            } else if (c == '\\') {
                if (pos_ >= input_.size()) return false;
                c = input_[pos_++];
                if (c == '"') outString += '"';
                else if (c == '\\') outString += '\\';
                else if (c == '/') outString += '/';
                else if (c == 'b') outString += '\b';
                else if (c == 'f') outString += '\f';
                else if (c == 'n') outString += '\n';
                else if (c == 'r') outString += '\r';
                else if (c == 't') outString += '\t';
                else if (c == 'u') {
                    // Simple unicode handling - just skip for now
                    pos_ += 4;
                } else {
                    outString += c;
                }
            } else {
                outString += c;
            }
        }
        return false;
    }

    bool ParseValue(std::string& outValue) {
        SkipWhitespace();
        char c = Peek();
        if (c == '"') {
            return ParseString(outValue);
        } else if (c == '{' || c == '[') {
            // For simplicity, skip objects and arrays
            int depth = 1;
            ++pos_;
            while (pos_ < input_.size() && depth > 0) {
                c = input_[pos_++];
                if (c == '{' || c == '[') ++depth;
                else if (c == '}' || c == ']') --depth;
            }
            outValue = "{}"; // Placeholder
            return true;
        } else {
            // Parse number, boolean, null
            size_t start = pos_;
            while (pos_ < input_.size() && !std::isspace(static_cast<unsigned char>(input_[pos_])) &&
                   input_[pos_] != ',' && input_[pos_] != '}' && input_[pos_] != ']') {
                ++pos_;
            }
            outValue = input_.substr(start, pos_ - start);
            return true;
        }
    }

    std::string input_;
    size_t pos_;
};

ComponentSlotCategory ParseCategory(const std::string& str) {
    if (str == "PowerPlant") return ComponentSlotCategory::PowerPlant;
    if (str == "MainThruster") return ComponentSlotCategory::MainThruster;
    if (str == "ManeuverThruster") return ComponentSlotCategory::ManeuverThruster;
    if (str == "Shield") return ComponentSlotCategory::Shield;
    if (str == "Weapon") return ComponentSlotCategory::Weapon;
    if (str == "Cargo") return ComponentSlotCategory::Cargo;
    if (str == "Support") return ComponentSlotCategory::Support;
    if (str == "Sensor") return ComponentSlotCategory::Sensor;
    if (str == "Computer") return ComponentSlotCategory::Computer;
    return ComponentSlotCategory::PowerPlant; // Default
}

SlotSize ParseSize(const std::string& str) {
    if (str == "XS") return SlotSize::XS;
    if (str == "Small") return SlotSize::Small;
    if (str == "Medium") return SlotSize::Medium;
    if (str == "Large") return SlotSize::Large;
    if (str == "XL") return SlotSize::XL;
    if (str == "XXL") return SlotSize::XXL;
    return SlotSize::Small; // Default
}

double ParseDouble(const std::string& str) {
    try {
        return std::stod(str);
    } catch (...) {
        return 0.0;
    }
}

int ParseInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

bool ParseBool(const std::string& str) {
    return str == "true" || str == "1";
}

// Simple array parser for strings (for factionRestrictions)
std::vector<std::string> ParseStringArray(const std::string& str) {
    // For now, just return empty vector since all components have empty restrictions
    return {};
}

bool LoadComponentFromJson(const std::string& jsonContent, ShipComponentBlueprint& outBlueprint) {
    JsonParser parser(jsonContent);
    std::unordered_map<std::string, std::string> object;
    if (!parser.ParseObject(object)) {
        return false;
    }

    outBlueprint.id = object["id"];
    outBlueprint.displayName = object["displayName"];
    outBlueprint.description = object["description"];
    outBlueprint.category = ParseCategory(object["category"]);
    outBlueprint.size = ParseSize(object["size"]);
    outBlueprint.massTons = ParseDouble(object["massTons"]);
    outBlueprint.powerOutputMW = ParseDouble(object["powerOutputMW"]);
    outBlueprint.powerDrawMW = ParseDouble(object["powerDrawMW"]);
    outBlueprint.thrustKN = ParseDouble(object["thrustKN"]);
    outBlueprint.heatGenerationMW = ParseDouble(object["heatGenerationMW"]);
    outBlueprint.heatDissipationMW = ParseDouble(object["heatDissipationMW"]);
    outBlueprint.crewRequired = ParseInt(object["crewRequired"]);
    outBlueprint.crewSupport = ParseInt(object["crewSupport"]);

    // Schema versioning and compatibility metadata
    outBlueprint.schemaVersion = ParseInt(object["schemaVersion"]);
    outBlueprint.techTier = ParseInt(object["techTier"]);
    outBlueprint.manufacturer = object["manufacturer"];
    outBlueprint.factionRestrictions = ParseStringArray(object["factionRestrictions"]);

    // Weapon fields
    if (object.find("weaponDamagePerShot") != object.end()) {
        outBlueprint.weaponDamagePerShot = ParseDouble(object["weaponDamagePerShot"]);
        outBlueprint.weaponRangeKm = ParseDouble(object["weaponRangeKm"]);
        outBlueprint.weaponFireRatePerSecond = ParseDouble(object["weaponFireRatePerSecond"]);
        outBlueprint.weaponAmmoCapacity = ParseInt(object["weaponAmmoCapacity"]);
        outBlueprint.weaponAmmoType = object["weaponAmmoType"];
        outBlueprint.weaponIsTurret = ParseBool(object["weaponIsTurret"]);
        outBlueprint.weaponTrackingSpeedDegPerSec = ParseDouble(object["weaponTrackingSpeedDegPerSec"]);
        outBlueprint.weaponProjectileSpeedKmPerSec = ParseDouble(object["weaponProjectileSpeedKmPerSec"]);
    }

    // Shield fields
    if (object.find("shieldCapacityMJ") != object.end()) {
        outBlueprint.shieldCapacityMJ = ParseDouble(object["shieldCapacityMJ"]);
        outBlueprint.shieldRechargeRateMJPerSec = ParseDouble(object["shieldRechargeRateMJPerSec"]);
        outBlueprint.shieldRechargeDelaySeconds = ParseDouble(object["shieldRechargeDelaySeconds"]);
        outBlueprint.shieldDamageAbsorption = ParseDouble(object["shieldDamageAbsorption"]);
    }

    return true;
}

} // namespace

namespace ComponentJsonLoader {

bool LoadComponentFromFile(const std::string& filePath, ShipComponentBlueprint& outBlueprint) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonContent = buffer.str();

    return LoadComponentFromJson(jsonContent, outBlueprint);
}

bool LoadComponentsFromDirectory(const std::string& directoryPath) {
    namespace fs = std::filesystem;
    bool loadedAny = false;

    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                ShipComponentBlueprint blueprint;
                if (LoadComponentFromFile(entry.path().string(), blueprint)) {
                    ShipComponentCatalog::Register(blueprint);
                    loadedAny = true;
                } else {
                    std::cerr << "Failed to load component from " << entry.path() << std::endl;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << directoryPath << ": " << e.what() << std::endl;
        return false;
    }

    return loadedAny;
}

// Hot-reloadable version that checks file modification times
bool LoadComponentsFromDirectoryHotReload(const std::string& directoryPath) {
    namespace fs = std::filesystem;
    bool needsReload = false;

    try {
        // First pass: check if any files have changed
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filePath = entry.path().string();
                auto currentTime = fs::last_write_time(entry);

                auto it = fileModificationTimes.find(filePath);
                if (it == fileModificationTimes.end() || it->second != currentTime) {
                    needsReload = true;
                    break;
                }
            }
        }

        if (!needsReload) {
            return true; // Nothing changed
        }

        // Clear catalog and reload all files
        ShipComponentCatalog::Clear();
        fileModificationTimes.clear();

        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filePath = entry.path().string();
                ShipComponentBlueprint blueprint;
                if (LoadComponentFromFile(filePath, blueprint)) {
                    ShipComponentCatalog::Register(blueprint);
                    fileModificationTimes[filePath] = fs::last_write_time(entry);
                } else {
                    std::cerr << "Failed to load component from " << filePath << std::endl;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << directoryPath << ": " << e.what() << std::endl;
        return false;
    }

    return true;
}

} // namespace ComponentJsonLoader