// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaArrayAccessor.h"

#include <sol/sol.hpp>

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<template<typename> class AccessorType>
  UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor(ChimeraTK::DataType type,
      Module* owner, const std::string& name, const std::string& unit, size_t nElements,
      const std::string& description, const std::unordered_set<std::string>& tags) {
    std::optional<UserTypeTemplateVariantNoVoid<ArrayAccessor>> rv;
    ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
      using UserType = decltype(t);
      AccessorType<UserType> acc(owner, name, unit, nElements, description, tags);
      rv.emplace(std::in_place_type<ArrayAccessor<UserType>>, std::move(acc));
    });
    return std::move(rv.value());
  }

  /********************************************************************************************************************/

  LuaArrayAccessor::~LuaArrayAccessor() = default;

  /********************************************************************************************************************/

  sol::object LuaArrayAccessor::getElement(sol::this_state s, int index) const {
    // Lua is 1-based; C++ buffer is 0-based.
    return std::visit(
        [&](auto& acc) -> sol::object {
          return sol::make_object(s, acc[static_cast<size_t>(index - 1)]);
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaArrayAccessor::setElement(int index, sol::object val) {
    std::visit(
        [&](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          acc[static_cast<size_t>(index - 1)] = val.as<T>();
        },
        _accessor);
  }

  /********************************************************************************************************************/

  size_t LuaArrayAccessor::getNElements() const {
    size_t n{0};
    std::visit([&](auto& acc) { n = acc.getNElements(); }, _accessor);
    return n;
  }

  /********************************************************************************************************************/

  LuaArrayAccessor& LuaArrayAccessor::readAndGet() {
    read();
    return *this; // Return the view itself for chaining: for i,v in pairs(arr:readAndGet()) do
  }

  /********************************************************************************************************************/

  void LuaArrayAccessor::bind(sol::state& lua) {
    lua.new_usertype<LuaArrayAccessor>("ArrayAccessor",
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
        sol::no_constructor,
        "read", &LuaArrayAccessor::read,
        "readNonBlocking", &LuaArrayAccessor::readNonBlocking,
        "readLatest", &LuaArrayAccessor::readLatest,
        "write", &LuaArrayAccessor::write,
        "writeDestructively", &LuaArrayAccessor::writeDestructively,
        "getNElements", &LuaArrayAccessor::getNElements,
        "getName", &LuaArrayAccessor::getName,
        "getUnit", &LuaArrayAccessor::getUnit,
        "getDescription", &LuaArrayAccessor::getDescription,
        "getValueType", &LuaArrayAccessor::getValueType,
        "getVersionNumber", &LuaArrayAccessor::getVersionNumber,
        "isReadOnly", &LuaArrayAccessor::isReadOnly,
        "isReadable", &LuaArrayAccessor::isReadable,
        "isWriteable", &LuaArrayAccessor::isWriteable,
        "getId", &LuaArrayAccessor::getId,
        "dataValidity", &LuaArrayAccessor::dataValidity,

        // View metamethods: arr[i] and arr[i] = v — directly into the C++ buffer, no copy.
        sol::meta_function::index, &LuaArrayAccessor::getElement,
        sol::meta_function::new_index, &LuaArrayAccessor::setElement,
        sol::meta_function::length, &LuaArrayAccessor::getNElements,

        // pairs/ipairs-compatible stateful iterator.
        // Returns (iterator_fn, arr, 0) so `for i, v in pairs(arr)` works.
        sol::meta_function::pairs,
        [](LuaArrayAccessor& arr) {
          auto iterFn = [](LuaArrayAccessor& a, int i, sol::this_state s) -> std::tuple<sol::object, sol::object> {
            int next = i + 1;
            if(static_cast<size_t>(next) > a.getNElements()) {
              return {sol::lua_nil, sol::lua_nil};
            }
            return {sol::make_object(s, next), a.getElement(s, next)};
          };
          return std::make_tuple(sol::as_function(iterFn), std::ref(arr), 0);
        });

    // Array accessor factory functions.
    lua.set_function("ArrayPushInput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPushInput>{}, type, &owner, name, unit, nElements, description);
        });
    lua.set_function("ArrayPushInputWB",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPushInputWB>{}, type, &owner, name, unit, nElements, description);
        });
    lua.set_function("ArrayPollInput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPollInput>{}, type, &owner, name, unit, nElements, description);
        });
    lua.set_function("ArrayOutput",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutput>{}, type, &owner, name, unit, nElements, description);
        });
    lua.set_function("ArrayOutputPushRB",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutputPushRB>{}, type, &owner, name, unit, nElements, description);
        });
    lua.set_function("ArrayOutputReverseRecovery",
        [](ChimeraTK::DataType type, VariableGroup& owner, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutputReverseRecovery>{}, type, &owner, name, unit, nElements, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
