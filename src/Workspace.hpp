#pragma once
#include <vector>
#include <memory>
#include "Column.hpp"

class Workspace {
public:
    Workspace() = default;
    
    // Core Layout access
    std::vector<Column>& getColumns() { return columns; }
    const std::vector<Column>& getColumns() const { return columns; }

    int getFocusedColumn() const { return focusedColumn; }
    int getFocusedRow() const { return focusedRow; }
    float getViewportOffset() const { return viewportOffset; }

    void setFocusedColumn(int c);
    void setFocusedRow(int r);
    void setViewportOffset(float offset);

    // Hierarchy additions
    void insertColumn(Column col, int index = -1);
    bool removeColumn(int index);
    
    void addWindowContextually(std::shared_ptr<Window> win);
    std::shared_ptr<Window> removeWindow(HWND hwnd);

    // Core Interaction API
    void consumeOrExpelLeft();
    void consumeOrExpelRight();
    void consumeIntoColumn();
    void expelFromColumn();

    void scroll(int deltaColumn);
    void scrollVertical(int deltaRow);
    void moveFocusedWindow(int deltaColumn);
    void moveFocusedWindowVertical(int deltaRow);
    
    void cycleActiveColumnWidth();
    void cycleActiveWindowHeight();
    void toggleFullscreenOnFocused();

    // Query 
    bool hasWindow(HWND hwnd) const;
    bool setFocusToWindow(HWND hwnd);
    std::shared_ptr<Window> getFocusedWindow() const;
    
    bool isEmpty() const { return columns.empty(); }
    void ensureFocusLimits();

private:
    std::vector<Column> columns;
    int focusedColumn = 0;
    int focusedRow = 0;
    float viewportOffset = 0.0f;

    static float getNextPresetScale(float currentScale);
};
