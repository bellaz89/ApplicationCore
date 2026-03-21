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

    // Factory function: ModuleGroup(owner, name, description)
    // Returns a reference — ownership is managed on the C++ side by the parent LuaOwningObject.
    lua.set_function("ModuleGroup",
        [](LuaModuleGroup& owner, const std::string& name, const std::string& description) -> LuaModuleGroup& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaModuleGroup>(&owner, name, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
