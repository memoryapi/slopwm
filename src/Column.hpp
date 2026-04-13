#pragma once
#include <vector>
#include <memory>
#include "Window.hpp"

class Column {
public:
    Column() = default;
    Column(std::shared_ptr<Window> win, float widthScale = 0.5f);
    
    // Copy/Move constructors
    Column(const Column&) = default;
    Column& operator=(const Column&) = default;
    Column(Column&&) noexcept = default;
    Column& operator=(Column&&) noexcept = default;

    std::vector<std::shared_ptr<Window>>& getWindows() { return windows; }
    const std::vector<std::shared_ptr<Window>>& getWindows() const { return windows; }

    float getWidthScale() const { return widthScale; }
    void setWidthScale(float scale) { widthScale = scale; }

    // Insertion & Deletion
    void insertWindow(std::shared_ptr<Window> win, int index = -1);
    std::shared_ptr<Window> removeWindow(int index);
    
    // Mathematical normalization
    void cycleWindowHeight(int index);
    void equalizeHeights();

private:
    std::vector<std::shared_ptr<Window>> windows;
    float widthScale = 0.5f;

    static float getNextPresetScale(float currentScale);
};
