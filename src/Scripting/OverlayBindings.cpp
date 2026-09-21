#include "OverlayBindings.h"

#include <GUI/Overlay/OverlaySystem.h>
#include <GUI/Overlay/ScriptedOverlay.h>

#include "ImGuiBindings.h"

#include "Library/Logger/Logger.h"

#include <memory>

OverlayBindings::OverlayBindings(OverlaySystem &overlaySystem) : _overlaySystem(overlaySystem) {
}

sol::table OverlayBindings::createBindingTable(sol::state_view &solState) const {
    sol::table table = solState.create_table_with(
        "addOverlay", sol::as_function([this, &solState](std::string_view name, sol::table view) {
#ifdef __vita__
            // Creating an overlay goes through ImGui, so log it and keep a failure from taking the process down.
            MM_INFO("Adding scripted overlay '{}'.", name);
            try {
                _overlaySystem.addOverlay(name, std::make_unique<ScriptedOverlay>(name, solState, view));
                MM_INFO("Scripted overlay '{}' added.", name);
            } catch (const std::exception &e) {
                MM_ERROR("Couldn't add overlay '{}': {}", name, e.what());
            }
#else
            _overlaySystem.addOverlay(name, std::make_unique<ScriptedOverlay>(name, solState, view));
#endif
        }),
        "removeOverlay", sol::as_function([this](std::string_view name) {
            _overlaySystem.removeOverlay(name);
        })
    );
    ImGuiBindings::Init(solState, table);

    return table;
}
