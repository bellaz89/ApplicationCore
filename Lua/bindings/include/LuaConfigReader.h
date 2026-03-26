// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ConfigReader.h"

#include <functional>
#include <list>
#include <string>
#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Lua wrapper around ConfigReader.
   *
   * This exposes the same XML configuration lookup semantics to Lua modules as
   * appConfig() exposes to C++ modules.
   */
  class LuaConfigReader {
   public:
    /** Construct a Lua wrapper for the given application configuration reader. */
    explicit LuaConfigReader(ConfigReader& reader) : _reader(reader) {}

    /** Read a scalar configuration value. */
    sol::object get(const std::string& path, sol::object defaultValue, sol::this_state s);

    /** Read an array configuration value. */
    sol::object getArray(const std::string& path, sol::object defaultValue, sol::this_state s);

    /** Return the names of all modules below the given XML path. */
    [[nodiscard]] std::list<std::string> getModules(const std::string& path) { return _reader.get().getModules(path); }

    /** Register the Lua config reader bindings into the given Lua state. */
    static void bind(sol::state& lua);

   private:
    std::reference_wrapper<ConfigReader> _reader;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
