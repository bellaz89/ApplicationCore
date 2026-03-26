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

  /**
   * Lua-backed ApplicationModule implementation.
   *
   * The Lua script constructs this object from the shared loading state and
   * assigns the module mainLoop function to a property. The binding captures
   * the function bytecode and migratable state, then reconstructs the module in
   * its dedicated runtime Lua state when the module thread starts.
   */
  class LuaApplicationModule : public ApplicationModule, public LuaOwningObject {
   public:
    /**
     * Construct a Lua-backed ApplicationModule.
     *
     * The Lua script assigns the main loop afterwards via
     * mod.mainLoop = function(...) ... end.
     */
    LuaApplicationModule(ModuleGroup* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    LuaApplicationModule(LuaApplicationModule&&) = default;

    /** Execute the reconstructed Lua mainLoop in the module thread. */
    void mainLoop() override;

    /** Create the per-module Lua state and prepare execution. */
    void run() override;

    /** Interrupt the running Lua module and release its runtime state. */
    void terminate() override;

    /** Register the Lua ApplicationModule bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Build a state-migration factory for a Lua value captured during loading. */
    static std::function<sol::object(sol::state_view)> makeFactory(sol::object val);

   private:
    /** Capture the mainLoop bytecode and all migratable upvalues. */
    void captureMainLoop(sol::protected_function& fn);

    /** Extract all upvalues from the function at funcIdx on the Lua stack. */
    static std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>> extractUpvalues(
        lua_State* L, int funcIdx);

    /** Bytecode of the user-supplied mainLoop function captured from the loading state. */
    std::vector<char> _mainLoopBytecode;

    /** Per-module runtime Lua state, accessed only from the module thread. */
    std::unique_ptr<sol::state> _moduleState;

    /** mainLoop function rebound into _moduleState. Valid only after run() has been called. */
    sol::protected_function _mainLoopFn;

    /**
     * Storage for all module properties captured in the loading state.
     *
     * Maps property names to factory functions which reconstruct the value in a
     * target runtime VM.
     */
    std::unordered_map<std::string, std::function<sol::object(sol::state_view)>> _properties;

    /** Upvalue factories for mainLoop in original upvalue index order. */
    std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>> _mainLoopUpvalues;

    /** Mutex protecting _moduleState from concurrent access during shutdown. */
    std::mutex _mutex;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
