#pragma once
#include "WorkspaceManager.hpp"
#include <windows.h>

class LayoutRenderer {
public:
    static void renderActiveWorkspace(WorkspaceManager& manager, RECT workArea);
    static void parkInactiveWorkspaces(const WorkspaceManager& manager, RECT workArea);
};
