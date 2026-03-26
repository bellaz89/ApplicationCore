// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <memory>

namespace ChimeraTK {

  class Application;

  namespace detail {
    struct LuaModuleManagerImpl;
  }

  /**
   * Loads and manages Lua-based ApplicationModules declared in the ConfigReader XML file under
   * the \<LuaModules\> section.
   *
   * The Application keeps one instance and calls createModules() in its constructor. The Lua loading
   * state (sol::state) is kept alive for the duration of the Application so all registered C++ objects
   * remain valid.
   */
  class LuaModuleManager {
   public:
    /** Construct an empty Lua module manager. */
    LuaModuleManager();
    /** Destroy the manager and release any remaining Lua state. */
    ~LuaModuleManager();

    /**
     * Load all Lua modules listed in the application config under \<LuaModules\>.
     *
     * Must be called from the Application constructor before initialise().
     */
    void createModules(Application& app);

    /**
     * Terminate all Lua ApplicationModule threads and release the Lua state.
     *
     * Called from Application::shutdown().
     */
    void deinit();

   private:
    void init(Application& app);

    std::unique_ptr<detail::LuaModuleManagerImpl> _impl;
  };

} // namespace ChimeraTK
