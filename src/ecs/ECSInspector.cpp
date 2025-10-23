#include "ECSInspector.h"

#include "TypeNameUtils.h"
#include "../TextRenderer.h"
#include "../Viewport3D.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

#if defined(USE_GLFW)
// Define CALLBACK for GLU compatibility on MinGW
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#include <glad/glad.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#endif

namespace {
constexpr int kInspectorPanelWidth = 440;
constexpr int kInspectorMaxRows = 14;
}

ECSInspector::ECSInspector()
    : entityManager_(nullptr)
    , enabled_(false)
    , selectedFilterIndex_(0)
    , totalEntities_(0)
    , shownEntities_(0)
    , lastRefresh_(std::chrono::steady_clock::now())
    , refreshInterval_(std::chrono::duration<double>(0.25))
    , consolePrintedForFrame_(false) {
    filters_.push_back({"All Components", std::nullopt});
    instructionLine_ = "Toggle [I] • Prev [ [ • Next ] ] • Clear [0]";
}

void ECSInspector::SetEntityManager(EntityManager* manager) {
    entityManager_ = manager;
    if (enabled_) {
        lastRefresh_ = std::chrono::steady_clock::time_point::min();
        RefreshData();
    }
}

void ECSInspector::Toggle() {
    SetEnabled(!enabled_);
}

void ECSInspector::SetEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    consolePrintedForFrame_ = false;
    if (enabled_) {
        lastRefresh_ = std::chrono::steady_clock::time_point::min();
        RefreshData();
    }
}

void ECSInspector::NextFilter() {
    if (filters_.empty()) {
        return;
    }
    selectedFilterIndex_ = (selectedFilterIndex_ + 1) % filters_.size();
    BuildDisplayRows();
    consolePrintedForFrame_ = false;
}

void ECSInspector::PreviousFilter() {
    if (filters_.empty()) {
        return;
    }
    if (selectedFilterIndex_ == 0) {
        selectedFilterIndex_ = filters_.size() - 1;
    } else {
        --selectedFilterIndex_;
    }
    BuildDisplayRows();
    consolePrintedForFrame_ = false;
}

void ECSInspector::ClearFilter() {
    if (filters_.empty()) {
        return;
    }
    selectedFilterIndex_ = 0;
    BuildDisplayRows();
    consolePrintedForFrame_ = false;
}

void ECSInspector::Render(Viewport3D& viewport) {
    if (!enabled_) {
        consolePrintedForFrame_ = false;
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - lastRefresh_ >= refreshInterval_) {
        RefreshData();
        lastRefresh_ = now;
    }

    DrawOverlay(viewport);
}

void ECSInspector::RefreshData() {
    consolePrintedForFrame_ = false;
    totalEntities_ = 0;
    shownEntities_ = 0;

    entityTypeCache_.clear();

    std::unordered_set<std::type_index> uniqueTypes;

    if (entityManager_) {
        entityManager_->EnumerateEntities([&](Entity entity, const std::vector<std::type_index>& types) {
            entityTypeCache_.emplace_back(entity, types);
            for (const auto& type : types) {
                uniqueTypes.insert(type);
            }
            ++totalEntities_;
        });
    }

    std::sort(entityTypeCache_.begin(), entityTypeCache_.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    std::vector<std::pair<std::string, std::type_index>> sortedTypes;
    sortedTypes.reserve(uniqueTypes.size());
    for (const auto& type : uniqueTypes) {
        sortedTypes.emplace_back(FormatTypeName(type), type);
    }
    std::sort(sortedTypes.begin(), sortedTypes.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    filters_.clear();
    filters_.push_back({"All Components", std::nullopt});
    for (const auto& entry : sortedTypes) {
        filters_.push_back({entry.first, entry.second});
    }

    EnsureFilterValid();
    BuildDisplayRows();
}

void ECSInspector::EnsureFilterValid() {
    if (filters_.empty()) {
        filters_.push_back({"All Components", std::nullopt});
    }
    if (selectedFilterIndex_ >= filters_.size()) {
        selectedFilterIndex_ = 0;
    }
}

void ECSInspector::BuildDisplayRows() {
    displayRows_.clear();
    shownEntities_ = 0;

    EnsureFilterValid();
    const auto& filter = filters_[selectedFilterIndex_];
    const bool includeAll = !filter.type.has_value();

    for (const auto& [entity, types] : entityTypeCache_) {
        bool include = includeAll;
        if (!includeAll && !types.empty()) {
            include = std::find(types.begin(), types.end(), *filter.type) != types.end();
        } else if (!includeAll && types.empty()) {
            include = false;
        }
        if (!include) {
            continue;
        }

        std::vector<std::string> componentNames;
        componentNames.reserve(types.size());
        for (const auto& type : types) {
            componentNames.push_back(FormatTypeName(type));
        }
        std::sort(componentNames.begin(), componentNames.end());

        std::ostringstream row;
        row << "Entity " << entity << " (" << componentNames.size() << ")";
        if (componentNames.empty()) {
            row << ": (no components)";
        } else {
            row << ": ";
            for (size_t i = 0; i < componentNames.size(); ++i) {
                if (i != 0) {
                    row << ", ";
                }
                row << componentNames[i];
            }
        }

        displayRows_.push_back(row.str());
        ++shownEntities_;
    }

    std::ostringstream header;
    header << "ECS Inspector — Filter: " << filter.name
           << " (" << shownEntities_ << '/' << totalEntities_ << ')';
    headerLine_ = header.str();
}

std::string ECSInspector::FormatTypeName(const std::type_index& typeIndex) {
    auto it = typeNameCache_.find(typeIndex);
    if (it != typeNameCache_.end()) {
        return it->second;
    }
    std::string name = ecs::debug::GetReadableTypeName(typeIndex);
    typeNameCache_.emplace(typeIndex, name);
    return name;
}

void ECSInspector::DrawOverlay(Viewport3D& viewport) {
#if defined(USE_GLFW)
    if (!viewport.usingGL()) {
        DrawConsoleFallback();
        return;
    }

    GLFWwindow* window = viewport.GetGLFWWindow();
    if (!window) {
        DrawConsoleFallback();
        return;
    }

    glfwMakeContextCurrent(window);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, viewport.GetWidth(), viewport.GetHeight(), 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int fontHeight = TextRenderer::GetFontHeight(FontSize::Fixed);
    if (fontHeight <= 0) {
        fontHeight = 14;
    }

    int linesToShow = static_cast<int>(displayRows_.size());
    bool truncated = false;
    if (linesToShow > kInspectorMaxRows) {
        linesToShow = kInspectorMaxRows;
        truncated = true;
    }

    int totalLines = 2 + linesToShow + (truncated ? 1 : 0);
    int panelHeight = 16 + totalLines * (fontHeight + 4);

    int originX = std::max(16, viewport.GetWidth() - kInspectorPanelWidth - 16);
    int originY = 32;

    glColor4f(0.05f, 0.05f, 0.08f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(static_cast<float>(originX), static_cast<float>(originY));
    glVertex2f(static_cast<float>(originX + kInspectorPanelWidth), static_cast<float>(originY));
    glVertex2f(static_cast<float>(originX + kInspectorPanelWidth), static_cast<float>(originY + panelHeight));
    glVertex2f(static_cast<float>(originX), static_cast<float>(originY + panelHeight));
    glEnd();

    glColor4f(0.2f, 0.6f, 0.9f, 0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(static_cast<float>(originX), static_cast<float>(originY));
    glVertex2f(static_cast<float>(originX + kInspectorPanelWidth), static_cast<float>(originY));
    glVertex2f(static_cast<float>(originX + kInspectorPanelWidth), static_cast<float>(originY + panelHeight));
    glVertex2f(static_cast<float>(originX), static_cast<float>(originY + panelHeight));
    glEnd();

    int textX = originX + 16;
    int textY = originY + fontHeight + 12;

    TextRenderer::RenderTextWithShadow(headerLine_, textX, textY, TextColor::Yellow(), TextColor::Black(), FontSize::Fixed);
    textY += fontHeight + 6;

    TextRenderer::RenderTextWithShadow(instructionLine_, textX, textY, TextColor::Gray(0.7f), TextColor::Black(), FontSize::Fixed);
    textY += fontHeight + 6;

    for (int i = 0; i < linesToShow; ++i) {
        TextRenderer::RenderTextWithShadow(displayRows_[i], textX, textY, TextColor::White(), TextColor::Black(), FontSize::Fixed);
        textY += fontHeight + 4;
    }

    if (truncated) {
        std::ostringstream more;
        more << "... (" << (displayRows_.size() - linesToShow) << " more entities)";
        TextRenderer::RenderTextWithShadow(more.str(), textX, textY, TextColor::Gray(0.6f), TextColor::Black(), FontSize::Fixed);
        textY += fontHeight + 4;
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
#else
    (void)viewport;
    DrawConsoleFallback();
#endif
}

void ECSInspector::DrawConsoleFallback() {
    if (consolePrintedForFrame_) {
        return;
    }
    consolePrintedForFrame_ = true;

    std::cout << "[ECS Inspector] " << headerLine_ << std::endl;
    const size_t previewCount = std::min<size_t>(displayRows_.size(), 5);
    for (size_t i = 0; i < previewCount; ++i) {
        std::cout << "  " << displayRows_[i] << std::endl;
    }
    if (displayRows_.size() > previewCount) {
        std::cout << "  ... (" << (displayRows_.size() - previewCount) << " more entities)" << std::endl;
    }
}

