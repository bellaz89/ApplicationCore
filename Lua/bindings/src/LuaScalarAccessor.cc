// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaScalarAccessor.h"

#include <sol/sol.hpp>

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<template<typename> class AccessorType>
  UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor(ChimeraTK::DataType type,
      Module* owner, const std::string& name, const std::string& unit, const std::string& description,
      const std::unordered_set<std::string>& tags) {
    std::optional<UserTypeTemplateVariantNoVoid<ScalarAccessor>> rv;
    ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
      using UserType = decltype(t);
      AccessorType<UserType> acc(owner, name, unit, description, tags);
      rv.emplace(std::in_place_type<ScalarAccessor<UserType>>, std::move(acc));
    });
    return std::move(rv.value());
  }

  /********************************************************************************************************************/

  LuaScalarAccessor::LuaScalarAccessor() : _accessor(ScalarOutput<int>()) {}

  /********************************************************************************************************************/

  template<template<typename> class AccessorType>
  LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<AccessorType>, ChimeraTK::DataType type, Module* owner,
      const std::string& name, const std::string& unit, const std::string& description,
      const std::unordered_set<std::string>& tags)
  : _accessor(createAccessor<AccessorType>(type, owner, name, unit, description, tags)) {}

  /********************************************************************************************************************/

  LuaScalarAccessor::~LuaScalarAccessor() = default;

  /********************************************************************************************************************/

  sol::object LuaScalarAccessor::get(sol::this_state s) const {
    return std::visit(
        [&s](auto& acc) -> sol::object {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          auto* impl = boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(acc.getHighLevelImplElement()).get();
          return sol::make_object(s, T(impl->accessData(0)));
        },
        _accessor);
  }

  /********************************************************************************************************************/

  sol::object LuaScalarAccessor::readAndGet(sol::this_state s) {
    visit([](auto& acc) { acc.read(); });
    return get(s);
  }

  /********************************************************************************************************************/

  void LuaScalarAccessor::set(sol::object val) {
    std::visit(
        [&val](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          acc = val.as<T>();
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaScalarAccessor::setAndWrite(sol::object val) {
    set(val);
    write();
  }

  /********************************************************************************************************************/

  void LuaScalarAccessor::writeIfDifferent(sol::object val) {
    std::visit(
        [&val](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          acc.writeIfDifferent(val.as<T>());
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaScalarAccessor::bind(sol::state& lua) {
    lua.new_usertype<LuaScalarAccessor>("ScalarAccessor",
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
        sol::no_constructor,
        "read", &LuaScalarAccessor::read,
        "readNonBlocking", &LuaScalarAccessor::readNonBlocking,
        "readLatest", &LuaScalarAccessor::readLatest,
        "write", &LuaScalarAccessor::write,
        "writeDestructively", &LuaScalarAccessor::writeDestructively,
        "get", &LuaScalarAccessor::get,
        "readAndGet", &LuaScalarAccessor::readAndGet,
        "set", &LuaScalarAccessor::set,
        "setAndWrite", &LuaScalarAccessor::setAndWrite,
        "writeIfDifferent", &LuaScalarAccessor::writeIfDifferent,
        "getName", &LuaScalarAccessor::getName,
        "getUnit", &LuaScalarAccessor::getUnit,
        "getDescription", &LuaScalarAccessor::getDescription,
        "getValueType", &LuaScalarAccessor::getValueType,
        "getVersionNumber", &LuaScalarAccessor::getVersionNumber,
        "isReadOnly", &LuaScalarAccessor::isReadOnly,
        "isReadable", &LuaScalarAccessor::isReadable,
        "isWriteable", &LuaScalarAccessor::isWriteable,
        "getId", &LuaScalarAccessor::getId,
        "dataValidity", &LuaScalarAccessor::dataValidity);

    // Scalar accessor factory functions — return reference; ownership is on the C++ side.
    lua.set_function("ScalarPushInput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPushInput>{}, type, &owner, name, unit, description);
        });
    lua.set_function("ScalarPushInputWB",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPushInputWB>{}, type, &owner, name, unit, description);
        });
    lua.set_function("ScalarPollInput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPollInput>{}, type, &owner, name, unit, description);
        });
    lua.set_function("ScalarOutput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutput>{}, type, &owner, name, unit, description);
        });
    lua.set_function("ScalarOutputPushRB",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutputPushRB>{}, type, &owner, name, unit, description);
        });
    lua.set_function("ScalarOutputReverseRecovery",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutputReverseRecovery>{}, type, &owner, name, unit, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
