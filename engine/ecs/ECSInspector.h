#pragma once

#include "EntityManager.h"

#include <chrono>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class Viewport3D;

struct ECSInspectorFilterOption {
    std::string name;
    std::optional<std::type_index> type;
};

class ECSInspector {
public:
    ECSInspector();

    void SetEntityManager(EntityManager* manager);

    void Toggle();
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }

    void NextFilter();
    void PreviousFilter();
    void ClearFilter();

    void Render(Viewport3D& viewport);

private:
    void RefreshData();
    void EnsureFilterValid();
    void BuildDisplayRows();
    void DrawOverlay(Viewport3D& viewport);
    void DrawConsoleFallback();

    std::string FormatTypeName(const std::type_index& typeIndex);

    EntityManager* entityManager_;
    bool enabled_;
    size_t selectedFilterIndex_;
    std::vector<ECSInspectorFilterOption> filters_;
    std::vector<std::string> displayRows_;
    std::string headerLine_;
    std::string instructionLine_;
    size_t totalEntities_;
    size_t shownEntities_;

    std::unordered_map<std::type_index, std::string> typeNameCache_;
    std::vector<std::pair<Entity, std::vector<std::type_index>>> entityTypeCache_;

    std::chrono::steady_clock::time_point lastRefresh_;
    std::chrono::duration<double> refreshInterval_;

    bool consolePrintedForFrame_;
};

