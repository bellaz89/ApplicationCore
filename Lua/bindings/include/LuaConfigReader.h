// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ConfigReader.h"

#include <ChimeraTK/VariantUserTypes.h>

#include <functional>
#include <list>
#include <string>

namespace sol {
  class state;
  struct this_state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaConfigReader {
   public:
    explicit LuaConfigReader(ConfigReader& reader) : _reader(reader) {}

    sol::object get(ChimeraTK::DataType dt, const std::string& path, sol::object defaultValue, sol::this_state s);

    sol::object getArray(
        ChimeraTK::DataType dt, const std::string& path, sol::object defaultValue, sol::this_state s);

    [[nodiscard]] std::list<std::string> getModules(const std::string& path) { return _reader.get().getModules(path); }

    static void bind(sol::state& lua);

   private:
    std::reference_wrapper<ConfigReader> _reader;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
