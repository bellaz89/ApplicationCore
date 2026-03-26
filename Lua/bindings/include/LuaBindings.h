// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /**
   * Register all ChimeraTK ApplicationCore bindings into the given Lua state.
   *
   * Called by LuaModuleManager::init() for the shared loading state, and by
   * LuaApplicationModule::run() for each per-module execution state.
   */
  void registerLuaBindings(sol::state& lua);

} // namespace ChimeraTK
