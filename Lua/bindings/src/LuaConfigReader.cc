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

    // New API: config global table with auto-type inference
    auto configTable = lua.create_table();

    // config.get(path, default) — type inferred from default value type
    // config.get(DataType, path, default) — explicit type (delegates to LuaConfigReader)
    configTable.set_function("get",
        sol::overload(
            // explicit DataType form
            [](ChimeraTK::DataType dt, const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
              LuaConfigReader cr(ApplicationModule::appConfig());
              return cr.get(dt, path, defVal, s);
            },
            // auto-type from default value
            [](const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
              auto& reader = ApplicationModule::appConfig();
              sol::state_view sv(s);
              if(defVal.get_type() == sol::type::string) {
                std::string def = defVal.as<std::string>();
                return sol::make_object(sv, reader.get<std::string>(path, def));
              }
              else if(defVal.get_type() == sol::type::boolean) {
                bool def = defVal.as<bool>();
                return sol::make_object(sv, reader.get<ChimeraTK::Boolean>(path, def));
              }
              else {
                // number or nil default → float64
                if(defVal == sol::lua_nil) {
                  return sol::make_object(sv, reader.get<double>(path));
                }
                double def = defVal.as<double>();
                return sol::make_object(sv, reader.get<double>(path, def));
              }
            }));

    // config.getArray(path, default_table) — type inferred from first element
    // config.getArray(DataType, path, default) — explicit type
    configTable.set_function("getArray",
        sol::overload(
            // explicit DataType
            [](ChimeraTK::DataType dt, const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
              LuaConfigReader cr(ApplicationModule::appConfig());
              return cr.getArray(dt, path, defVal, s);
            },
            // auto-type: infer from first element of default table
            [](const std::string& path, sol::object defVal, sol::this_state s) -> sol::object {
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
                else {
                  std::vector<double> def;
                  for(int i = 1; i <= static_cast<int>(tbl.size()); ++i) def.push_back(tbl.get<double>(i));
                  auto vec = reader.get<std::vector<double>>(path, def);
                  sol::table out = sv.create_table(static_cast<int>(vec.size()));
                  for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = vec[i];
                  return out;
                }
              }
              else {
                auto vec = reader.get<std::vector<double>>(path);
                sol::table out = sv.create_table(static_cast<int>(vec.size()));
                for(size_t i = 0; i < vec.size(); ++i) out[i + 1] = vec[i];
                return out;
              }
            }));

    configTable.set_function("getModules", [](const std::string& path) {
      return ApplicationModule::appConfig().getModules(path);
    });

    lua["config"] = configTable;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
