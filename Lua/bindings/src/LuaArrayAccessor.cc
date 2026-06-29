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
          const size_t n = acc.getNElements();
          if(index < 1 || static_cast<size_t>(index) > n) {
            luaL_error(s, "array index %d out of range [1, %zu]", index, n);
            return sol::object{}; // unreachable; luaL_error longjmps
          }
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          // sol2 has no registered conversion for ChimeraTK::Boolean; route through bool.
          if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
            return sol::make_object(s, static_cast<bool>(acc[static_cast<size_t>(index - 1)]));
          }
          else {
            return sol::make_object(s, acc[static_cast<size_t>(index - 1)]);
          }
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaArrayAccessor::setElement(int index, sol::object val) {
    std::visit(
        [&](auto& acc) {
          const size_t n = acc.getNElements();
          if(index < 1 || static_cast<size_t>(index) > n) {
            luaL_error(val.lua_state(), "array index %d out of range [1, %zu]", index, n);
            return; // unreachable; luaL_error longjmps
          }
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          // sol2 has no registered conversion for ChimeraTK::Boolean; route through bool.
          if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
            acc[static_cast<size_t>(index - 1)] = val.as<bool>();
          }
          else {
            acc[static_cast<size_t>(index - 1)] = val.as<T>();
          }
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

  void LuaArrayAccessor::fillTable(sol::table t, std::optional<size_t> maxN) const {
    // Single std::visit for the whole range: one C++ dispatch regardless of n.
    std::visit(
        [&](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          const size_t accN = static_cast<size_t>(acc.getNElements());
          const size_t n = maxN.has_value() ? std::min(maxN.value(), accN) : accN;
          for(size_t i = 0; i < n; ++i) {
            if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
              t[i + 1] = static_cast<bool>(acc[i]);
            }
            else {
              t[i + 1] = acc[i];
            }
          }
        },
        _accessor);
  }

  /********************************************************************************************************************/

  sol::table LuaArrayAccessor::toTable(sol::this_state s) const {
    // Delegate to fillTable so both paths share the same inner loop.
    sol::state_view sv(s);
    sol::table t = sv.create_table(static_cast<int>(getNElements()), 0);
    fillTable(t, std::nullopt);
    return t;
  }

  /********************************************************************************************************************/

  void LuaArrayAccessor::fromTable(sol::table t) {
    // Single std::visit for the whole array: avoids one dispatch per element
    // compared to calling setElement() in a Lua loop.
    std::visit(
        [&](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          using T = typename ACC::value_type;
          const size_t n = acc.getNElements();
          for(size_t i = 0; i < n; ++i) {
            if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
              acc[i] = t.get<bool>(static_cast<int>(i + 1));
            }
            else {
              acc[i] = t.get<T>(static_cast<int>(i + 1));
            }
          }
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaArrayAccessor::bind(sol::state& lua) {
    lua.new_usertype<LuaArrayAccessor>("ArrayAccessor",
        sol::no_constructor,
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
        "read", &LuaArrayAccessor::read,
        "readNonBlocking", &LuaArrayAccessor::readNonBlocking,
        "readLatest", &LuaArrayAccessor::readLatest,
        "readAndGet", &LuaArrayAccessor::readAndGet,
        "toTable", &LuaArrayAccessor::toTable,
        "fillTable", &LuaArrayAccessor::fillTable,
        "fromTable", &LuaArrayAccessor::fromTable,
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
        sol::meta_function::length, &LuaArrayAccessor::getNElements
    );

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


  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayPushInput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayPushInputWB>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayPollInput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayOutput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayOutputPushRB>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ArrayAccessor> LuaArrayAccessor::createAccessor<ArrayOutputReverseRecovery>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);

  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayPushInput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayPushInputWB>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayPollInput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayOutput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayOutputPushRB>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);
  template LuaArrayAccessor::LuaArrayAccessor(AccessorTypeTag<ArrayOutputReverseRecovery>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, size_t, const std::string&, const std::unordered_set<std::string>&);

} // namespace ChimeraTK
