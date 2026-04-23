// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ApplicationModule.h"
#include "LuaOwnershipManagement.h"

#include <sol/sol.hpp>

#include <memory>
#include <string>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Lua-backed ApplicationModule implementation.
   *
   * Each instance owns its own sol::state. The script is loaded and executed during construction
   * (before Application::initialise()), so the accessors it declares are registered with this
   * module in time for the control-system PV manager to discover them. run() then spawns the
   * module thread, which re-enters the Lua state to invoke mainLoop. The script phase and the
   * mainLoop phase execute in different threads, but strictly sequentially — no concurrent access.
   */
  class LuaApplicationModule : public ApplicationModule, public LuaOwningObject {
   public:
    /**
     * Construct a Lua-backed ApplicationModule.
     *
     * @param owner The parent ModuleGroup
     * @param name The module name
     * @param description The module description
     * @param scriptPath The path to the Lua script file to execute
     */
    LuaApplicationModule(ModuleGroup* owner, const std::string& name, const std::string& description,
        const std::string& scriptPath);

    LuaApplicationModule(LuaApplicationModule&&) = default;

    /** Execute the Lua mainLoop function in the module thread. */
    void mainLoop() override;

    /**
     * Invoke the Lua-side `prepare` function if the script defined one. Called by the framework
     * before runApplication() completes; intended for initial-value writes that must satisfy
     * push-input consumers at startup. No-op if the script did not define prepare.
     */
    void prepare() override;

    /** Start the module thread. The Lua script was already executed at construction time. */
    void run() override;

    /** Interrupt the running Lua module and release its runtime state. */
    void terminate() override;

    /** Register the Lua ApplicationModule bindings into the given Lua state. */
    static void bind(sol::state& lua);

   private:
    /** Create the per-module Lua state, execute the script, and cache mainLoop. */
    void loadScript();

    /** Path to the Lua script file to execute. */
    std::string _scriptPath;

    /** Per-module runtime Lua state. Populated in the constructor, then used by mainLoop(). */
    std::unique_ptr<sol::state> _moduleState;

    /** mainLoop function extracted from the script during construction. */
    sol::protected_function _mainLoopFn;

    /** Optional prepare function extracted from the script. May be empty (script did not define one). */
    sol::protected_function _prepareFn;

    /**
     * Companion Lua table holding dynamically-assigned fields on the module userdata
     * (e.g. `mod.myOutput = ...`, `function mod:mainLoop()`). Accessed via the __index /
     * __newindex fallbacks registered in bind(). Fields that collide with registered
     * members take precedence — sol2 only invokes the fallbacks for unknown keys.
     */
    sol::table _attrs;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
