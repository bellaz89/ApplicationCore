// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaModuleManager.h"

#include "Application.h"
#include "ConfigReader.h"
#include "LuaApplicationModule.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaModuleManager::createModules(Application& app) {
    auto& config = app.getConfigReader();
    for(auto& module : config.getModules("LuaModules")) {
      auto scriptPath = config.get<std::string>("LuaModules/" + module + "/path");

      // LuaApplicationModule self-registers with the Application via the ApplicationModule base
      // ctor. Application is itself a ModuleGroup, so it is a valid owner. The module takes
      // ownership of executing the Lua script on its own thread during run().
      new LuaApplicationModule(&app, module, "Lua module", scriptPath);
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
