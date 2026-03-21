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

  class LuaModuleGroup : public ModuleGroup, public LuaOwningObject {
   public:
    using ModuleGroup::ModuleGroup;
    LuaModuleGroup(LuaModuleGroup&&) = default;

    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
