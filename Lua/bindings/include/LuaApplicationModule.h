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
    /// Construct a module; the Lua script assigns mainLoop via mod.mainLoop = function(...)
    LuaApplicationModule(ModuleGroup* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    LuaApplicationModule(LuaApplicationModule&&) = default;

    void mainLoop() override;
    void run() override;
    void terminate() override;

    static void bind(sol::state& lua);

    /// Build a factory for any migratable Lua value.
    static std::function<sol::object(sol::state_view)> makeFactory(sol::object val);

   private:
    /// Capture mainLoop bytecode + upvalues from a sol::protected_function.
    void captureMainLoop(sol::protected_function& fn);

    /// Extract all upvalues from the function at funcIdx on L's stack.
    static std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>> extractUpvalues(
        lua_State* L, int funcIdx);

    /// Bytecode of the user-supplied mainLoop function, captured at construction from the loading state.
    std::vector<char> _mainLoopBytecode;

    /// Per-module Lua state created in run(). Only accessed from the module's own C++ thread.
    std::unique_ptr<sol::state> _moduleState;

    /// The mainLoop function, bound into _moduleState. Only valid after run() is called.
    sol::protected_function _mainLoopFn;

    /// Storage for all module properties (accessors, scalars, strings, tables, functions).
    /// Maps property names to factory functions that reconstruct the value in the target VM.
    std::unordered_map<std::string, std::function<sol::object(sol::state_view)>> _properties;

    /// Upvalue factories for mainLoop: (name, factory) pairs, in upvalue index order.
    std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>> _mainLoopUpvalues;

    /// Mutex protecting _moduleState from concurrent access (e.g. during terminate()).
    std::mutex _mutex;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
