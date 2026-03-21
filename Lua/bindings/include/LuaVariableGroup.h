// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "VariableGroup.h"

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaVariableGroup : public VariableGroup, public LuaOwningObject {
   public:
    using VariableGroup::VariableGroup;
    LuaVariableGroup(LuaVariableGroup&&) = default;

    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
