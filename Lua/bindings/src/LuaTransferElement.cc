// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaTransferElement.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaTransferElementBase::bind(sol::state& lua) {
    // Register as a base usertype so sol2 knows about it for inheritance.
    // Concrete derived types (ScalarAccessor, ArrayAccessor, VoidAccessor) are registered separately.
    lua.new_usertype<LuaTransferElementBase>("TransferElementBase", sol::no_constructor);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
