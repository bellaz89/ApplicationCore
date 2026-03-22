// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaModuleGroup.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaModuleGroup::bind(sol::state& lua) {
    lua.new_usertype<LuaModuleGroup>("ModuleGroup",
        sol::no_constructor,
        "getName", &LuaModuleGroup::getName);

    // Factory function: ModuleGroup(owner, name, description) or ModuleGroup(name, description)
    // Returns a reference — ownership is managed on the C++ side by the parent LuaOwningObject.
    lua.set_function("ModuleGroup",
        sol::overload(
            // With explicit owner
            [](LuaModuleGroup& owner, const std::string& name,
                const std::string& description) -> LuaModuleGroup& {
              return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaModuleGroup>(&owner, name, description);
            },
            // Attach to app root (use "app" global which is set by manager)
            [](const std::string& name, const std::string& description,
                sol::this_state s) -> LuaModuleGroup& {
              sol::state_view sv(s);
              LuaModuleGroup& root = sv["app"];
              return *dynamic_cast<LuaOwningObject&>(root).make_child<LuaModuleGroup>(&root, name, description);
            }));
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
