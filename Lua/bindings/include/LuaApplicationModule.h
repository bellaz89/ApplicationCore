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
   * Each Lua script executes directly in the module's dedicated Lua state during module startup.
   * The script is loaded and executed in the module thread (via run()), which simplifies the
   * architecture by avoiding state migration between threads.
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
        const std::string& scriptPath, const std::unordered_set<std::string>& tags = {});

    LuaApplicationModule(LuaApplicationModule&&) = default;

    /** Execute the Lua mainLoop function in the module thread. */
    void mainLoop() override;

    /** Create the per-module Lua state and execute the Lua script. */
    void run() override;

    /** Interrupt the running Lua module and release its runtime state. */
    void terminate() override;

    /** Register the Lua ApplicationModule bindings into the given Lua state. */
    static void bind(sol::state& lua);

   private:
    /** Path to the Lua script file to execute. */
    std::string _scriptPath;

    /** Per-module runtime Lua state, accessed only from the module thread. */
    std::unique_ptr<sol::state> _moduleState;

    /** mainLoop function extracted from the Lua script execution. Valid only after run() has been called. */
    sol::protected_function _mainLoopFn;

    /** Mutex protecting _moduleState from concurrent access during shutdown. */
    std::mutex _mutex;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
