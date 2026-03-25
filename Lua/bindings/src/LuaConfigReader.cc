// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaConfigReader.h"

#include "ApplicationModule.h"

#include <sol/sol.hpp>

#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  sol::object LuaConfigReader::get(const std::string& path, sol::object defaultValue, sol::this_state s) {
    auto& reader = _reader.get();
    sol::state_view sv(s);
    if(defaultValue.get_type() == sol::type::string) {
      std::string def = defaultValue.as<std::string>();
      return sol::make_object(sv, reader.get<std::string>(path, def));
    }
    if(defaultValue.get_type() == sol::type::boolean) {
      bool def = defaultValue.as<bool>();
      return sol::make_object(sv, reader.get<ChimeraTK::Boolean>(path, def));
    }
    if(defaultValue == sol::lua_nil) {
      return sol::make_object(sv, reader.get<double>(path));
    }
    double def = defaultValue.as<double>();
    return sol::make_object(sv, reader.get<double>(path, def));
  }

  /********************************************************************************************************************/

  sol::object LuaConfigReader::getArray(const std::string& path, sol::object defaultValue, sol::this_state s) {
    auto& reader = _reader.get();
    sol::state_view sv(s);
    if(defaultValue.get_type() == sol::type::table) {
      sol::table tbl = defaultValue.as<sol::table>();
      sol::object first = tbl[1];
      if(first.get_type() == sol::type::string) {
        std::vector<std::string> def;
        for(int i = 1; i <= static_cast<int>(tbl.size()); ++i) def.push_back(tbl.get<std::string>(i));
        auto vec = reader.get<std::vector<std::string>>(path, def);
        sol::table out = sv.create_table(static_cast<int>(vec.size()));
        for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = vec[i];
        return out;
      }
      if(first.get_type() == sol::type::boolean) {
        std::vector<ChimeraTK::Boolean> def;
        for(int i = 1; i <= static_cast<int>(tbl.size()); ++i) def.push_back(tbl.get<bool>(i));
        auto vec = reader.get<std::vector<ChimeraTK::Boolean>>(path, def);
        sol::table out = sv.create_table(static_cast<int>(vec.size()));
        for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = static_cast<bool>(vec[i]);
        return out;
      }
      std::vector<double> def;
      for(int i = 1; i <= static_cast<int>(tbl.size()); ++i) def.push_back(tbl.get<double>(i));
      auto vec = reader.get<std::vector<double>>(path, def);
      sol::table out = sv.create_table(static_cast<int>(vec.size()));
      for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = vec[i];
      return out;
    }

    auto vec = reader.get<std::vector<double>>(path);
    sol::table out = sv.create_table(static_cast<int>(vec.size()));
    for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = vec[i];
    return out;
  }

  /********************************************************************************************************************/

  void LuaConfigReader::bind(sol::state& lua) {
    lua.new_usertype<LuaConfigReader>("ConfigReader",
        sol::no_constructor,
        "get", &LuaConfigReader::get,
        "getArray", &LuaConfigReader::getArray,
        "getModules", &LuaConfigReader::getModules);

    lua.set_function("appConfig", []() {
      return LuaConfigReader(ApplicationModule::appConfig());
    });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
