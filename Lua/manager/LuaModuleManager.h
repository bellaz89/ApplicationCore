// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

namespace ChimeraTK {

  class Application;

  /**
   * Loads Lua-based ApplicationModules declared in the ConfigReader XML file under
   * the \<LuaModules\> section.
   *
   * Each module is created directly under the Application; its Lua script is executed
   * from the module's own run() method.
   */
  class LuaModuleManager {
   public:
    LuaModuleManager() = default;

    /**
     * Instantiate all Lua modules listed in the application config under \<LuaModules\>.
     *
     * Must be called from the Application constructor before initialise().
     */
    void createModules(Application& app);

    /**
     * No-op retained for symmetry with PythonModuleManager::deinit(). Lua modules are
     * ApplicationModules and are terminated by Application::shutdown() via the normal
     * module-lifecycle path, so this manager holds no extra state to release.
     */
    void deinit() {}
  };

} // namespace ChimeraTK
