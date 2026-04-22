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

namespace ChimeraTK {

  /********************************************************************************************************************/

  namespace detail {

    struct LuaModuleManagerImpl {
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

    _impl->mainGroup.reset();
    _impl.reset();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
