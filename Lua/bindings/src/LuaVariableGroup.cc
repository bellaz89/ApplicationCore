// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaVariableGroup.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaVariableGroup::bind(sol::state& lua) {
    lua.new_usertype<LuaVariableGroup>("VariableGroup",
        sol::no_constructor,
        "getName", &LuaVariableGroup::getName,
        "readAll", [](LuaVariableGroup& self, bool includeReturnChannels) {
          self.readAll(includeReturnChannels);
        },
        "readAllLatest", [](LuaVariableGroup& self, bool includeReturnChannels) {
          self.readAllLatest(includeReturnChannels);
        },
        "readAllNonBlocking", [](LuaVariableGroup& self, bool includeReturnChannels) {
          self.readAllNonBlocking(includeReturnChannels);
        },
        "writeAll", [](LuaVariableGroup& self, bool includeReturnChannels) {
          self.writeAll(includeReturnChannels);
        },
        "writeAllDestructively", [](LuaVariableGroup& self, bool includeReturnChannels) {
          self.writeAllDestructively(includeReturnChannels);
        });

    // Factory function: VariableGroup(owner, name, description)
    // owner may be a LuaApplicationModule or a LuaVariableGroup — both inherit LuaOwningObject
    // via VariableGroup. We accept VariableGroup& as the common base.
    lua.set_function("VariableGroup",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaVariableGroup& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaVariableGroup>(&owner, name, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
