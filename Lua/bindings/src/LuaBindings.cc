// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaBindings.h"

#include "LuaApplicationModule.h"
#include "LuaArrayAccessor.h"
#include "LuaConfigReader.h"
#include "LuaDataConsistencyGroup.h"
#include "LuaExtraModules.h"
#include "LuaLogger.h"
#include "LuaModuleGroup.h"
#include "LuaReadAnyGroup.h"
#include "LuaScalarAccessor.h"
#include "LuaStatusAccessor.h"
#include "LuaTransferElement.h"
#include "LuaVariableGroup.h"
#include "LuaVoidAccessor.h"

#include <ChimeraTK/DataType.h>
#include <ChimeraTK/TransferElementID.h>

#include <sol/sol.hpp>

#include <sstream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void registerLuaBindings(sol::state& lua) {
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math, sol::lib::io, sol::lib::os,
        sol::lib::debug);

    // ---- DataType -------------------------------------------------------
    // Register the C++ type so sol2 knows its metatable (needed to pass DataType values to C++ factory functions).
    lua.new_usertype<ChimeraTK::DataType>("_DataType",
        sol::no_constructor,
        sol::meta_function::equal_to,
        [](const DataType& a, const DataType& b) { return a == b; },
        sol::meta_function::to_string,
        [](const DataType& dt) { return std::string(dt); });

    // Expose as a table of named constants (DataType userdata objects).
    // Replacing the global after new_usertype is safe: the metatable stays in the Lua registry.
    lua["DataType"] = lua.create_table_with(
        "int8", DataType{DataType::int8},
        "uint8", DataType{DataType::uint8},
        "int16", DataType{DataType::int16},
        "uint16", DataType{DataType::uint16},
        "int32", DataType{DataType::int32},
        "uint32", DataType{DataType::uint32},
        "int64", DataType{DataType::int64},
        "uint64", DataType{DataType::uint64},
        "float32", DataType{DataType::float32},
        "float64", DataType{DataType::float64},
        "string", DataType{DataType::string},
        "Boolean", DataType{DataType::Boolean},
        "Void", DataType{DataType::Void});

    // ---- DataValidity ----------------------------------------------------
    lua.new_enum<DataValidity>("DataValidity", "ok", DataValidity::ok, "faulty", DataValidity::faulty);

    // ---- VersionNumber ---------------------------------------------------
    lua.new_usertype<VersionNumber>("VersionNumber",
        sol::no_constructor,
        "isNullVersion",
        &VersionNumber::isNullVersion,
        sol::meta_function::equal_to,
        [](const VersionNumber& a, const VersionNumber& b) { return a == b; },
        sol::meta_function::less_than,
        [](const VersionNumber& a, const VersionNumber& b) { return a < b; },
        sol::meta_function::less_than_or_equal_to,
        [](const VersionNumber& a, const VersionNumber& b) { return a <= b; },
        sol::meta_function::to_string,
        [](const VersionNumber& v) -> std::string {
          std::ostringstream ss;
          ss << v;
          return ss.str();
        });

    // Constructor function: VersionNumber() → new unique version; VersionNumber(nil) → null version.
    lua.set_function("VersionNumber",
        sol::overload([]() { return VersionNumber{}; }, [](sol::lua_nil_t) { return VersionNumber{nullptr}; }));

    // ---- TransferElementID -----------------------------------------------
    lua.new_usertype<TransferElementID>("TransferElementID",
        sol::no_constructor,
        "isValid",
        &TransferElementID::isValid,
        sol::meta_function::equal_to,
        [](const TransferElementID& a, const TransferElementID& b) { return a == b; },
        sol::meta_function::to_string,
        [](const TransferElementID& id) -> std::string {
          std::ostringstream ss;
          ss << id;
          return ss.str();
        });

    // ---- Accessor / module bindings ------------------------------------
    LuaTransferElementBase::bind(lua);
    LuaModuleGroup::bind(lua);
    LuaVariableGroup::bind(lua);
    LuaApplicationModule::bind(lua);
    LuaScalarAccessor::bind(lua);
    LuaArrayAccessor::bind(lua);
    LuaVoidAccessor::bind(lua);
    LuaReadAnyGroup::bind(lua);
    LuaDataConsistencyGroup::bind(lua);
    LuaLogger::bind(lua);
    LuaConfigReader::bind(lua);
    LuaStatusAccessor::bind(lua);
    LuaExtraModules::bind(lua);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
