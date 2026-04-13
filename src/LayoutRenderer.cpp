#include "LayoutRenderer.hpp"
#include "Utils.hpp"
#include <cmath>
#include <algorithm>

void LayoutRenderer::renderActiveWorkspace(WorkspaceManager& manager, RECT workArea) {
    if (manager.getWorkspaces().empty()) return;
    auto& ws = manager.getActiveWorkspace();
    const auto& columns = ws.getColumns();
    if (columns.empty()) return;

    int monWidth = workArea.right - workArea.left;
    int monHeight = workArea.bottom - workArea.top;

    std::vector<float> colVirtualX(columns.size());
    float currentVirtualX = 0.0f;
    for (size_t c = 0; c < columns.size(); ++c) {
        colVirtualX[c] = currentVirtualX;
        currentVirtualX += columns[c].getWidthScale();
    }

    int focusedColumn = ws.getFocusedColumn();
    float focusedStart = colVirtualX[focusedColumn];
    float focusedWidth = columns[focusedColumn].getWidthScale();
    float focusedEnd = focusedStart + focusedWidth;

    float viewportOffset = ws.getViewportOffset();
    if (focusedStart < viewportOffset) {
        viewportOffset = focusedStart;
    }
    if (focusedEnd > viewportOffset + 1.0f) {
        viewportOffset = focusedEnd - 1.0f;
        if (focusedStart < viewportOffset) {
            viewportOffset = focusedStart; 
        }
    }

    float maxViewport = (std::max)(0.0f, currentVirtualX - 1.0f);
    viewportOffset = std::clamp(viewportOffset, 0.0f, maxViewport);
    ws.setViewportOffset(viewportOffset);

    int totalWindows = 0;
    for (const auto& col : columns) {
        totalWindows += col.getWindows().size();
    }

    HDWP hdwp = BeginDeferWindowPos(totalWindows);
    if (!hdwp) {
        utils::debugLog("BeginDeferWindowPos failed");
        return;
    }

    int offscreenX = workArea.left + 50000;
    int offscreenY = workArea.top + 50000;

    for (int c = 0; c < static_cast<int>(columns.size()); ++c) {
        const auto& col = columns[c];
        const auto& wins = col.getWindows();
        
        float startX = colVirtualX[c];
        float endX = startX + col.getWidthScale();
        bool inViewport = (startX < viewportOffset + 1.0f && endX > viewportOffset);
        
        float totalHeightScale = 0.0f;
        for (const auto& win : wins) totalHeightScale += win->getHeightScale();

        int currentY = workArea.top;
        for (int r = 0; r < static_cast<int>(wins.size()); ++r) {
            const auto& win = wins[r];
            if (!win->isValid()) continue;

            float fraction = win->getHeightScale() / (totalHeightScale > 0.0f ? totalHeightScale : 1.0f);
            int winHeight = static_cast<int>(std::round(fraction * monHeight));
            
            if (r == static_cast<int>(wins.size()) - 1) winHeight = workArea.bottom - currentY;

            int pixelWidth = static_cast<int>(std::round(col.getWidthScale() * monWidth));

            if (inViewport) {
                float relStart = startX - viewportOffset;
                int pixelX = workArea.left + static_cast<int>(std::round(relStart * monWidth));
                win->setPos(pixelX, currentY, pixelWidth, winHeight, hdwp);
            } else {
                win->setPos(offscreenX, offscreenY, pixelWidth, winHeight, hdwp);
            }
            currentY += winHeight;
        }
    }
    EndDeferWindowPos(hdwp);
}

void LayoutRenderer::parkInactiveWorkspaces(const WorkspaceManager& manager, RECT workArea) {
    int activeIndex = manager.getActiveIndex();
    const auto& workspaces = manager.getWorkspaces();
    
    int offscreenX = workArea.left + 50000;
    int offscreenY = workArea.top + 50000;
    int monWidth = workArea.right - workArea.left;
    int monHeight = workArea.bottom - workArea.top;

    for (size_t wi = 0; wi < workspaces.size(); ++wi) {
        if (static_cast<int>(wi) == activeIndex) continue;
        
        const auto& ws = workspaces[wi];
        int wsWins = 0;
        for (const auto& col : ws.getColumns()) wsWins += col.getWindows().size();
        if (wsWins == 0) continue;
        
        HDWP hdwp = BeginDeferWindowPos(wsWins);
        if (!hdwp) continue;
        
        for (const auto& col : ws.getColumns()) {
            float totalHeightScale = 0.0f;
            for (const auto& win : col.getWindows()) totalHeightScale += win->getHeightScale();

            int pixelWidth = static_cast<int>(std::round(col.getWidthScale() * monWidth));
            int currentY = workArea.top;

            const auto& wins = col.getWindows();
            for (int r = 0; r < static_cast<int>(wins.size()); ++r) {
                const auto& win = wins[r];
                float fraction = win->getHeightScale() / (totalHeightScale > 0.0f ? totalHeightScale : 1.0f);
                int winHeight = static_cast<int>(std::round(fraction * monHeight));
                
                if (r == static_cast<int>(wins.size()) - 1) winHeight = workArea.bottom - currentY;

                win->setPos(offscreenX, offscreenY, pixelWidth, winHeight, hdwp);
                currentY += winHeight;
            }
        }
        EndDeferWindowPos(hdwp);
    }
}
