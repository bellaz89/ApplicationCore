// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /** Register additional ApplicationCore modules exposed to Lua. */
  class LuaExtraModules {
   public:
    /** Register the extra module factories into the given Lua state. */
    static void bind(sol::state& lua);
  };

} // namespace ChimeraTK
