// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ApplicationModule.h"
#include "LuaOwnershipManagement.h"

#include <sol/sol.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaApplicationModule : public ApplicationModule, public LuaOwningObject {
   public:
    LuaApplicationModule(ModuleGroup* owner, const std::string& name, const std::string& description,
        sol::protected_function mainLoopFn, const std::unordered_set<std::string>& tags = {});

    LuaApplicationModule(LuaApplicationModule&&) = default;

    void mainLoop() override;
    void run() override;
    void terminate() override;

    static void bind(sol::state& lua);

   private:
    /// Bytecode of the user-supplied mainLoop function, captured at construction from the loading state.
    /// Loaded into the per-module sol::state when run() is called.
    std::vector<char> _mainLoopBytecode;

    /// Per-module Lua state created in run(). Only accessed from the module's own C++ thread.
    std::unique_ptr<sol::state> _moduleState;

    /// The mainLoop function, bound into _moduleState. Only valid after run() is called.
    sol::protected_function _mainLoopFn;

    /// Named accessor/submodule storage. Maps property names to factory functions that produce Lua userdatas.
    /// Populated via __newindex in the loading state; accessed via __index in the module VM.
    /// Using C++ factories so the lookup works across VM boundaries.
    std::unordered_map<std::string, std::function<sol::object(sol::state_view)>> _namedAccessors;

    /// Mutex protecting _moduleState from concurrent access (e.g. during terminate()).
    /// Part of the mutex tree: LuaModuleManager (root) -> LuaApplicationModule (leaf).
    std::mutex _mutex;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
