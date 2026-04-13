#include "Column.hpp"
#include <algorithm>
#include <array>

constexpr std::array<float, 3> PRESET_SCALES = { 1.0f / 3.0f, 0.5f, 2.0f / 3.0f };

float Column::getNextPresetScale(float currentScale) {
    for (float scale : PRESET_SCALES) {
        if (scale > currentScale + 0.01f) {
            return scale;
        }
    }
    return PRESET_SCALES[0];
}

Column::Column(std::shared_ptr<Window> win, float widthScale) : widthScale(widthScale) {
    if (win) {
        win->setHeightScale(1.0f);
        windows.push_back(std::move(win));
    }
}

void Column::insertWindow(std::shared_ptr<Window> win, int index) {
    if (!win) return;
    
    if (windows.empty()) {
        win->setHeightScale(1.0f);
        windows.push_back(std::move(win));
        return;
    }
    
    float totalH = 0.0f;
    for (const auto& w : windows) {
        totalH += w->getHeightScale();
    }
    win->setHeightScale(totalH / windows.size());
    
    if (index < 0 || index >= static_cast<int>(windows.size())) {
        windows.push_back(std::move(win));
    } else {
        windows.insert(windows.begin() + index, std::move(win));
    }
}

std::shared_ptr<Window> Column::removeWindow(int index) {
    if (index < 0 || index >= static_cast<int>(windows.size())) return nullptr;
    
    auto win = windows[index];
    windows.erase(windows.begin() + index);
    return win;
}

void Column::equalizeHeights() {
    float defaultScale = 1.0f / (windows.empty() ? 1.0f : static_cast<float>(windows.size()));
    for (auto& w : windows) {
        w->setHeightScale(defaultScale);
    }
}

void Column::cycleWindowHeight(int index) {
    if (windows.size() <= 1 || index < 0 || index >= static_cast<int>(windows.size())) return;

    float totalHeightScale = 0.0f;
    for (const auto& win : windows) {
        totalHeightScale += win->getHeightScale();
    }
    
    if (totalHeightScale > 0.0f) {
        for (const auto& win : windows) {
            win->setHeightScale(win->getHeightScale() / totalHeightScale);
        }
    }
    
    auto win = windows[index];
    float currentProp = win->getHeightScale();
    float targetProp = getNextPresetScale(currentProp);
    win->setHeightScale(targetProp);
    
    float remainingProp = (std::max)(0.0f, 1.0f - targetProp);
    float oldRemainingProp = (std::max)(0.0001f, 1.0f - currentProp);
    
    for (int r = 0; r < static_cast<int>(windows.size()); ++r) {
        if (r != index) {
            float oldProp = windows[r]->getHeightScale();
            float newProp = oldProp * (remainingProp / oldRemainingProp);
            windows[r]->setHeightScale(newProp);
        }
    }
}
