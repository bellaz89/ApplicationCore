// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaScalarAccessor.h"

#include <sol/sol.hpp>

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
          // Use operator T() directly on the accessor — avoids getHighLevelImplElement()
          // and the dynamic_pointer_cast + atomic refcount bumps on every read.
          if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
            return sol::make_object(s, static_cast<bool>(acc));
          }
          else {
            return sol::make_object(s, static_cast<T>(acc));
          }
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
          if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
            acc = val.as<bool>();
          }
          else {
            acc = val.as<T>();
          }
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
          if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
            acc.writeIfDifferent(val.as<bool>());
          }
          else {
            acc.writeIfDifferent(val.as<T>());
          }
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaScalarAccessor::bind(sol::state& lua) {
    // Helper lambda: get current value of an accessor as double
    auto getDouble = [](const LuaScalarAccessor& a) -> double {
      return std::visit(
          [](auto& acc) {
            using ACC = std::remove_reference_t<decltype(acc)>;
            using T = typename ACC::value_type;
            if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
              return static_cast<double>(static_cast<bool>(acc));
            }
            else if constexpr(std::is_same_v<T, std::string>) {
              return 0.0;
            }
            else {
              return static_cast<double>(static_cast<T>(acc));
            }
          },
          a._accessor);
    };

    lua.new_usertype<LuaScalarAccessor>("ScalarAccessor",
        sol::no_constructor,
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
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
        "dataValidity", &LuaScalarAccessor::dataValidity,

        // Arithmetic metamethods
        sol::meta_function::addition,
        sol::overload(
            [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) + getDouble(b));
            },
            [getDouble](const LuaScalarAccessor& a, double b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) + b);
            }),
        sol::meta_function::subtraction,
        sol::overload(
            [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) - getDouble(b));
            },
            [getDouble](const LuaScalarAccessor& a, double b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) - b);
            }),
        sol::meta_function::multiplication,
        sol::overload(
            [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) * getDouble(b));
            },
            [getDouble](const LuaScalarAccessor& a, double b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) * b);
            }),
        sol::meta_function::division,
        sol::overload(
            [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) / getDouble(b));
            },
            [getDouble](const LuaScalarAccessor& a, double b, sol::this_state s) -> sol::object {
              sol::state_view sv(s);
              return sol::make_object(sv, getDouble(a) / b);
            }),
        sol::meta_function::unary_minus,
        [getDouble](const LuaScalarAccessor& a, sol::this_state s) -> sol::object {
          sol::state_view sv(s);
          return sol::make_object(sv, -getDouble(a));
        },
        sol::meta_function::less_than,
        [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b) -> bool {
          return getDouble(a) < getDouble(b);
        },
        sol::meta_function::less_than_or_equal_to,
        [getDouble](const LuaScalarAccessor& a, const LuaScalarAccessor& b) -> bool {
          return getDouble(a) <= getDouble(b);
        },
        sol::meta_function::to_string,
        [getDouble](const LuaScalarAccessor& a) -> std::string {
          std::string name = a.getName();
          std::string validity = (a.dataValidity() == DataValidity::ok) ? "ok" : "faulty";
          std::string valStr;
          std::visit(
              [&](auto& acc) {
                using ACC = std::remove_reference_t<decltype(acc)>;
                using T = typename ACC::value_type;
                if constexpr(std::is_same_v<T, std::string>) {
                  valStr = static_cast<T>(acc);
                }
                else if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
                  valStr = static_cast<bool>(acc) ? "true" : "false";
                }
                else {
                  valStr = std::to_string(static_cast<T>(acc));
                }
              },
              a._accessor);
          return "<ScalarAccessor name=" + name + " value=" + valStr + " validity=" + validity + ">";
        });

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


  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarPushInput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarPushInputWB>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarPollInput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarOutput>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarOutputPushRB>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template UserTypeTemplateVariantNoVoid<ScalarAccessor> LuaScalarAccessor::createAccessor<ScalarOutputReverseRecovery>(
      ChimeraTK::DataType, Module*, const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);

  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarPushInput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarPushInputWB>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarPollInput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarOutput>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarOutputPushRB>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaScalarAccessor::LuaScalarAccessor(AccessorTypeTag<ScalarOutputReverseRecovery>, ChimeraTK::DataType, Module*,
      const std::string&, const std::string&, const std::string&, const std::unordered_set<std::string>&);

} // namespace ChimeraTK
