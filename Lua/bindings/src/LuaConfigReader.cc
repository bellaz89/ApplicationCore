// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaConfigReader.h"

#include "ApplicationModule.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  sol::object LuaConfigReader::get(
      ChimeraTK::DataType dt, const std::string& path, sol::object defaultValue, sol::this_state s) {
    sol::object result;
    ChimeraTK::callForTypeNoVoid(dt.getAsTypeInfo(), [&](auto t) {
      using UserType = decltype(t);
      if(defaultValue == sol::lua_nil) {
        result = sol::make_object(s, _reader.get().get<UserType>(path));
      }
      else {
        result = sol::make_object(s, _reader.get().get<UserType>(path, defaultValue.as<UserType>()));
      }
    });
    return result;
  }

  /********************************************************************************************************************/

  sol::object LuaConfigReader::getArray(
      ChimeraTK::DataType dt, const std::string& path, sol::object defaultValue, sol::this_state s) {
    sol::object result;
    ChimeraTK::callForTypeNoVoid(dt.getAsTypeInfo(), [&](auto t) {
      using UserType = decltype(t);
      if(defaultValue == sol::lua_nil) {
        auto vec = _reader.get().get<std::vector<UserType>>(path);
        sol::state_view sv{s};
        sol::table tbl = sv.create_table(static_cast<int>(vec.size()));
        for(size_t i = 0; i < vec.size(); ++i) {
          tbl[i + 1] = vec[i];
        }
        result = tbl;
      }
      else {
        auto defaultVec = defaultValue.as<std::vector<UserType>>();
        auto vec = _reader.get().get<std::vector<UserType>>(path, defaultVec);
        sol::state_view sv{s};
        sol::table tbl = sv.create_table(static_cast<int>(vec.size()));
        for(size_t i = 0; i < vec.size(); ++i) {
          tbl[i + 1] = vec[i];
        }
        result = tbl;
      }
    });
    return result;
  }

  /********************************************************************************************************************/

  void LuaConfigReader::bind(sol::state& lua) {
    lua.new_usertype<LuaConfigReader>("ConfigReader",
        sol::no_constructor,
        "get", &LuaConfigReader::get,
        "getArray", &LuaConfigReader::getArray,
        "getModules", &LuaConfigReader::getModules);

    lua.set_function("appConfig",
        []() { return LuaConfigReader(ApplicationModule::appConfig()); });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
