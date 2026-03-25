// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaConfigReader.h"

#include "ApplicationModule.h"

#include <sol/sol.hpp>

#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaConfigReader::bind(sol::state& lua) {
    auto configTable = lua.create_table();

    configTable.set_function("get", [](const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
      auto& reader = ApplicationModule::appConfig();
      sol::state_view sv(s);
      if(defVal.get_type() == sol::type::string) {
        std::string def = defVal.as<std::string>();
        return sol::make_object(sv, reader.get<std::string>(path, def));
      }
      if(defVal.get_type() == sol::type::boolean) {
        bool def = defVal.as<bool>();
        return sol::make_object(sv, reader.get<ChimeraTK::Boolean>(path, def));
      }
      if(defVal == sol::lua_nil) {
        return sol::make_object(sv, reader.get<double>(path));
      }
      double def = defVal.as<double>();
      return sol::make_object(sv, reader.get<double>(path, def));
    });

    configTable.set_function("getArray", [](const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
      auto& reader = ApplicationModule::appConfig();
      sol::state_view sv(s);
      if(defVal.get_type() == sol::type::table) {
        sol::table tbl = defVal.as<sol::table>();
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
    });

    configTable.set_function("getModules", [](const std::string& path) {
      return ApplicationModule::appConfig().getModules(path);
    });

    lua["xmlConfig"] = configTable;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
