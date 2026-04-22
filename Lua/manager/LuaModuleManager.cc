// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaModuleManager.h"

#include "Application.h"
#include "ConfigReader.h"
#include "LuaApplicationModule.h"
#include "LuaBindings.h"

#include <sol/sol.hpp>

#include <iostream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  namespace detail {

    struct LuaModuleManagerImpl {
      // No intermediate group needed - modules go directly under Application
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
    // Initialization is now minimal - modules are created directly under Application
  }

  /********************************************************************************************************************/

  void LuaModuleManager::createModules(Application& app) {
    auto& config = app.getConfigReader();
    for(auto& module : config.getModules("LuaModules")) {
      init(app);

      auto scriptPath = config.get<std::string>("LuaModules/" + module + "/path");

      std::cout << "LuaModuleManager: Creating module from " << scriptPath << std::endl;

      // Create the LuaApplicationModule directly under the Application with the script path.
      // The module will execute the script in its own thread during run().
      // Application is a ModuleGroup, so we can pass it directly as the owner.
      // The constructor automatically registers the module with the application.
      new LuaApplicationModule(&app, module, "Lua module", scriptPath);
    }
  }

  /********************************************************************************************************************/

  void LuaModuleManager::deinit() {
    if(!_impl) {
      return;
    }
    // Cleanup of impl struct - modules are managed by Application now
    _impl.reset();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
