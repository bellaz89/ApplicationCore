// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaDataConsistencyGroup.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaDataConsistencyGroup::bind(sol::state& lua) {
    lua.new_enum("MatchingMode",
        "none", DataConsistencyGroup::MatchingMode::none,
        "exact", DataConsistencyGroup::MatchingMode::exact,
        "historized", DataConsistencyGroup::MatchingMode::historized);

    lua.new_usertype<LuaDataConsistencyGroup>("DataConsistencyGroup",
        sol::constructors<LuaDataConsistencyGroup(DataConsistencyGroup::MatchingMode)>(),
        "add", sol::overload(
            [](LuaDataConsistencyGroup& self, LuaTransferElementBase& acc) { self.add(acc); },
            [](LuaDataConsistencyGroup& self, LuaTransferElementBase& acc, unsigned histLen) {
              self.add(acc, histLen);
            }),
        "update", &LuaDataConsistencyGroup::update,
        "getMatchingMode", &LuaDataConsistencyGroup::getMatchingMode);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
