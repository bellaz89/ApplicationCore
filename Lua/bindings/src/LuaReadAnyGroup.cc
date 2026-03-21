// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaReadAnyGroup.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaReadAnyGroup::bind(sol::state& lua) {
    lua.new_usertype<LuaReadAnyGroup>("ReadAnyGroup",
        sol::constructors<LuaReadAnyGroup()>(),
        "add", &LuaReadAnyGroup::add,
        "readAny", &LuaReadAnyGroup::readAny,
        "readAnyNonBlocking", &LuaReadAnyGroup::readAnyNonBlocking,
        "readUntil", sol::overload(
            sol::resolve<void(const TransferElementID&)>(&LuaReadAnyGroup::readUntil),
            sol::resolve<void(LuaTransferElementBase&)>(&LuaReadAnyGroup::readUntilAccessor)),
        "finalise", &LuaReadAnyGroup::finalise,
        "interrupt", &LuaReadAnyGroup::interrupt);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
