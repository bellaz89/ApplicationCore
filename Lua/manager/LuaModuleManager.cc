// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaModuleManager.h"

#include "Application.h"
#include "LuaBindings.h"
#include "LuaModuleGroup.h"

#include <sol/sol.hpp>

#include <iostream>
#include <mutex>

namespace ChimeraTK {

  /********************************************************************************************************************/

  namespace detail {

    struct LuaModuleManagerImpl {
      std::unique_ptr<sol::state> loadState;
      std::mutex loadMutex;
      std::unique_ptr<LuaModuleGroup> mainGroup;
    };

  } // namespace detail

  /********************************************************************************************************************/

  LuaModuleManager::LuaModuleManager() = default;

  /********************************************************************************************************************/

  LuaModuleManager::~LuaModuleManager() {
    if(_impl) {
      deinit();
    }
  }

  /********************************************************************************************************************/

  void LuaModuleManager::init(Application& app) {
    if(_impl) {
      return;
    }
    _impl = std::make_unique<detail::LuaModuleManagerImpl>();

    std::lock_guard<std::mutex> lock(_impl->loadMutex);

    // Create the loading Lua state and register all ChimeraTK bindings.
    _impl->loadState = std::make_unique<sol::state>();
    registerLuaBindings(*_impl->loadState);

    // Create the root module group that is exposed to Lua scripts as "app".
    _impl->mainGroup =
        std::make_unique<LuaModuleGroup>(&app, ".", "Root for Lua Modules");

    // Set the "app" global so scripts can attach modules to it.
    (*_impl->loadState)["app"] = std::ref(*_impl->mainGroup);
  }

  /********************************************************************************************************************/

  void LuaModuleManager::createModules(Application& app) {
    auto& config = app.getConfigReader();
    for(auto& module : config.getModules("LuaModules")) {
      init(app);

      auto path = config.get<std::string>("LuaModules/" + module + "/path");
      std::lock_guard<std::mutex> lock(_impl->loadMutex);

      std::cout << "LuaModuleManager: Loading script " << path << std::endl;

      auto result = _impl->loadState->script_file(path, [](lua_State* /*L*/, sol::protected_function_result pfr) {
        return pfr;
      });

      if(!result.valid()) {
        sol::error err = result;
        throw ChimeraTK::logic_error(std::string("Error loading Lua script '") + path + "': " + err.what());
      }
    }

    // All scripts have been executed and all C++ module/accessor objects are now owned by mainGroup.
    // The loading state is no longer needed — free its Lua heap (script globals, string table, etc.)
    // while leaving all C++ objects alive. Guard against the case where no LuaModules were configured
    // (loop never ran, _impl was never initialized).
    if(_impl) {
      _impl->loadState.reset();
    }
  }

  /********************************************************************************************************************/

  void LuaModuleManager::deinit() {
    if(!_impl) {
      return;
    }

    // Terminate all Lua ApplicationModule threads that may be running.
    if(_impl->mainGroup) {
      for(auto* mod : _impl->mainGroup->getSubmoduleListRecursive()) {
        mod->terminate();
      }
    }

    // loadState may have already been freed by createModules() after all scripts were loaded.
    // reset() on a null unique_ptr is a no-op, so this is always safe.
    _impl->loadState.reset();
    _impl->mainGroup.reset();
    _impl.reset();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
