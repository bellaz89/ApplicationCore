// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "ModuleGroup.h"

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Lua wrapper for ModuleGroup with C++-side ownership management. */
  class LuaModuleGroup : public ModuleGroup, public LuaOwningObject {
   public:
    using ModuleGroup::ModuleGroup;
    LuaModuleGroup(LuaModuleGroup&&) = default;

    /** Register the Lua module-group bindings into the given Lua state. */
    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
