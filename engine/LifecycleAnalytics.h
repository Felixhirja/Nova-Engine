#pragma once

#include "ActorLifecycleManager.h"
#include <unordered_map>
#include <mutex>
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>

namespace lifecycle {

// Simple analytics collector for actor lifecycle events
class LifecycleAnalytics {
public:
    static LifecycleAnalytics& Instance() {
        static LifecycleAnalytics inst;
        return inst;
    }

    // Hook registration
    void Initialize() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (initialized_) return;
        initialized_ = true;

        auto& manager = ActorLifecycleManager::Instance();

        // Count creations and record creation timestamp
        manager.RegisterHook(LifecycleEvent::PostCreate, "analytics_create",
            [this](LifecycleContext& ctx) {
                std::lock_guard<std::mutex> lk(mutex_);
                creationCountByType_[ctx.actorType]++;
                actorCreateTime_[ctx.actor] = std::chrono::high_resolution_clock::now();
                actorTypeByPtr_[ctx.actor] = ctx.actorType;
                actorNameByPtr_[ctx.actor] = ctx.actorName;
                totalCreations_++;
            });

        // Record initialization times
        manager.RegisterHook(LifecycleEvent::PostInitialize, "analytics_init",
            [this](LifecycleContext& ctx) {
                std::lock_guard<std::mutex> lk(mutex_);
                auto now = std::chrono::high_resolution_clock::now();
                auto it = actorCreateTime_.find(ctx.actor);
                if (it != actorCreateTime_.end()) {
                    double initDur = std::chrono::duration<double>(now - it->second).count();
                    initDurationsByType_[ctx.actorType].push_back(initDur);
                }
            });

        // Track active periods and destructions
        manager.RegisterHook(LifecycleEvent::PostActivate, "analytics_activate",
            [this](LifecycleContext& ctx) {
                std::lock_guard<std::mutex> lk(mutex_);
                activeStart_[ctx.actor] = std::chrono::high_resolution_clock::now();
                activeCountByType_[ctx.actorType]++;
            });

        manager.RegisterHook(LifecycleEvent::PostDestroy, "analytics_destroy",
            [this](LifecycleContext& ctx) {
                std::lock_guard<std::mutex> lk(mutex_);
                auto now = std::chrono::high_resolution_clock::now();
                auto it = activeStart_.find(ctx.actor);
                if (it != activeStart_.end()) {
                    double activeDur = std::chrono::duration<double>(now - it->second).count();
                    activeDurationsByType_[ctx.actorType].push_back(activeDur);
                    activeStart_.erase(it);
                }

                // Remove create time tracking
                actorCreateTime_.erase(ctx.actor);

                // Record destruction
                destructionCountByType_[ctx.actorType]++;
                actorTypeByPtr_.erase(ctx.actor);
                actorNameByPtr_.erase(ctx.actor);
            });

        // Event counts (generic)
        for (int ev = (int)LifecycleEvent::PreCreate; ev <= (int)LifecycleEvent::PostDestroy; ++ev) {
            LifecycleEvent e = static_cast<LifecycleEvent>(ev);
            std::string key = "analytics_event_" + std::to_string(ev);
            manager.RegisterHook(e, key,
                [this, e](LifecycleContext& ctx) {
                    std::lock_guard<std::mutex> lk(mutex_);
                    eventCountsByType_[ctx.actorType][e]++;
                });
        }
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!initialized_) return;
        initialized_ = false;
        // Clearing maps to release memory
        creationCountByType_.clear();
        initDurationsByType_.clear();
        activeDurationsByType_.clear();
        activeStart_.clear();
        actorCreateTime_.clear();
        eventCountsByType_.clear();
        actorTypeByPtr_.clear();
        actorNameByPtr_.clear();
    }

    // Generate a simple textual report
    std::string GenerateReport() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::ostringstream ss;
        ss << "=== Lifecycle Analytics Report ===\n";
        ss << "Total creations: " << totalCreations_ << "\n\n";

        ss << "Per-type summary:\n";
        for (const auto& [type, count] : creationCountByType_) {
            ss << "- " << type << ": created=" << count;
            auto initIt = initDurationsByType_.find(type);
            if (initIt != initDurationsByType_.end() && !initIt->second.empty()) {
                double sum = 0.0;
                for (double v : initIt->second) sum += v;
                ss << ", avg_init=" << (sum / initIt->second.size()) << "s";
            }
            auto activeIt = activeDurationsByType_.find(type);
            if (activeIt != activeDurationsByType_.end() && !activeIt->second.empty()) {
                double sum = 0.0;
                for (double v : activeIt->second) sum += v;
                ss << ", avg_active=" << (sum / activeIt->second.size()) << "s";
            }
            ss << "\n";
        }

        ss << "\nEvent counts by type:\n";
        for (const auto& [type, evmap] : eventCountsByType_) {
            ss << "- " << type << ": ";
            for (const auto& [ev, cnt] : evmap) {
                ss << static_cast<int>(ev) << "=" << cnt << " ";
            }
            ss << "\n";
        }

        ss << "\nActive actor snapshot:\n";
        for (const auto& [actorPtr, type] : actorTypeByPtr_) {
            auto nameIt = actorNameByPtr_.find(actorPtr);
            ss << "* " << (nameIt != actorNameByPtr_.end() ? nameIt->second : "<unknown>")
               << " (type=" << type << ")\n";
        }

        ss << "=================================\n";
        return ss.str();
    }

    void PrintReport() const {
        std::cout << GenerateReport();
    }

    // Export a minimal JSON-like report (no external deps)
    std::string ExportJson() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::ostringstream ss;
        ss << "{\n  \"totalCreations\": " << totalCreations_ << ",\n  \"types\": {\n";
        bool firstType = true;
        for (const auto& [type, count] : creationCountByType_) {
            if (!firstType) ss << ",\n";
            firstType = false;
            ss << "    \"" << Escape(type) << "\": { \"created\": " << count;
            auto initIt = initDurationsByType_.find(type);
            if (initIt != initDurationsByType_.end() && !initIt->second.empty()) {
                double sum = 0.0;
                for (double v : initIt->second) sum += v;
                ss << ", \"avg_init\": " << (sum / initIt->second.size());
            }
            auto activeIt = activeDurationsByType_.find(type);
            if (activeIt != activeDurationsByType_.end() && !activeIt->second.empty()) {
                double sum = 0.0;
                for (double v : activeIt->second) sum += v;
                ss << ", \"avg_active\": " << (sum / activeIt->second.size());
            }
            ss << " }";
        }
        ss << "\n  }\n}\n";
        return ss.str();
    }

private:
    LifecycleAnalytics() = default;

    static std::string Escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\\') out += "\\\\";
            else if (c == '"') out += "\\\"";
            else out += c;
        }
        return out;
    }

    mutable std::mutex mutex_;
    bool initialized_ = false;

    // Counters and metrics
    std::unordered_map<std::string, size_t> creationCountByType_;
    std::unordered_map<std::string, std::vector<double>> initDurationsByType_;
    std::unordered_map<std::string, std::vector<double>> activeDurationsByType_;
    std::unordered_map<std::string, size_t> destructionCountByType_;
    std::unordered_map<std::string, size_t> activeCountByType_;
    std::unordered_map<std::string, std::unordered_map<LifecycleEvent, size_t>> eventCountsByType_;

    // Per-actor tracking
    std::unordered_map<IActor*, std::chrono::high_resolution_clock::time_point> actorCreateTime_;
    std::unordered_map<IActor*, std::chrono::high_resolution_clock::time_point> activeStart_;
    std::unordered_map<IActor*, std::string> actorTypeByPtr_;
    std::unordered_map<IActor*, std::string> actorNameByPtr_;

    size_t totalCreations_ = 0;
};

// Helper to be called from LifecycleActor initialization function
inline void __InitializeLifecycleAnalytics() {
    LifecycleAnalytics::Instance().Initialize();
}

} // namespace lifecycle
