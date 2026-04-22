// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaModuleManager.h"

#include "Application.h"
#include "ConfigReader.h"
#include "LuaApplicationModule.h"
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

    // Create the root module group that holds all Lua modules.
    _impl->mainGroup =
        std::make_unique<LuaModuleGroup>(&app, ".", "Root for Lua Modules");
  }

  /********************************************************************************************************************/

  void LuaModuleManager::createModules(Application& app) {
    auto& config = app.getConfigReader();
    for(auto& module : config.getModules("LuaModules")) {
      init(app);

      auto scriptPath = config.get<std::string>("LuaModules/" + module + "/path");

      std::cout << "LuaModuleManager: Creating module from " << scriptPath << std::endl;

      // Create the LuaApplicationModule with the script path.
      // The module will execute the script in its own thread during run().
      auto& luaModule = *_impl->mainGroup->make_child<LuaApplicationModule>(
          _impl->mainGroup.get(), module, "Lua module", scriptPath);

      // The module is now owned by mainGroup and will be started by the Application.
    }

    // No per-module setup needed after createModules - all execution happens in module threads.
    // We keep mainGroup alive as it owns the modules, but we can free loadState if it was created.
    // Actually, we need to keep mainGroup but we can discard loadState references if needed.
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
