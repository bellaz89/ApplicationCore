// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaStatusAccessor.h"

#include "LuaModuleGroup.h"
#include "LuaVariableGroup.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  LuaStatusAccessor::LuaStatusAccessor() : _accessor(StatusOutput()) {}

  /********************************************************************************************************************/

  LuaStatusAccessor::LuaStatusAccessor(OutputTag, Module* owner, const std::string& name,
      const std::string& description, const std::unordered_set<std::string>& tags)
  : _accessor(StatusOutput(owner, name, description, tags)) {}

  /********************************************************************************************************************/

  LuaStatusAccessor::LuaStatusAccessor(PushInputTag, Module* owner, const std::string& name,
      const std::string& description, const std::unordered_set<std::string>& tags)
  : _accessor(StatusPushInput(owner, name, description, tags)) {}

  /********************************************************************************************************************/

  LuaStatusAccessor::LuaStatusAccessor(PollInputTag, Module* owner, const std::string& name,
      const std::string& description, const std::unordered_set<std::string>& tags)
  : _accessor(StatusPollInput(owner, name, description, tags)) {}

  /********************************************************************************************************************/

  LuaStatusAccessor::~LuaStatusAccessor() = default;

  /********************************************************************************************************************/

  int LuaStatusAccessor::get() const {
    return std::visit([](auto& acc) { return static_cast<int>(static_cast<StatusAccessorBase::Status>(acc)); },
        _accessor);
  }

  /********************************************************************************************************************/

  int LuaStatusAccessor::readAndGet() {
    // Single visit: fuse read() + get() to avoid paying the variant dispatch twice.
    return std::visit(
        [](auto& acc) {
          acc.read();
          return static_cast<int>(static_cast<StatusAccessorBase::Status>(acc));
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaStatusAccessor::set(int val) {
    std::visit([val](auto& acc) { acc = static_cast<StatusAccessorBase::Status>(val); }, _accessor);
  }

  /********************************************************************************************************************/

  void LuaStatusAccessor::setAndWrite(int val) {
    // Single visit: fuse set() + write() to avoid paying the variant dispatch twice.
    std::visit(
        [val](auto& acc) {
          acc = static_cast<StatusAccessorBase::Status>(val);
          acc.write();
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaStatusAccessor::writeIfDifferent(int val) {
    std::visit(
        [val](auto& acc) {
          using ACC = std::remove_reference_t<decltype(acc)>;
          if constexpr(std::is_same_v<ACC, StatusOutput>) {
            acc.writeIfDifferent(static_cast<StatusAccessorBase::Status>(val));
          }
        },
        _accessor);
  }

  /********************************************************************************************************************/

  void LuaStatusAccessor::bind(sol::state& lua) {
    // Status enum — use integer values from StatusAccessorBase::Status
    lua["Status"] = lua.create_table_with("OFF", static_cast<int>(StatusAccessorBase::Status::OFF), "OK",
        static_cast<int>(StatusAccessorBase::Status::OK), "WARNING",
        static_cast<int>(StatusAccessorBase::Status::WARNING), "FAULT",
        static_cast<int>(StatusAccessorBase::Status::FAULT));

    lua.new_usertype<LuaStatusAccessor>("StatusAccessor",
        sol::no_constructor,
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
        "read", &LuaStatusAccessor::read,
        "readNonBlocking", &LuaStatusAccessor::readNonBlocking,
        "readLatest", &LuaStatusAccessor::readLatest,
        "write", &LuaStatusAccessor::write,
        "get", &LuaStatusAccessor::get,
        "readAndGet", &LuaStatusAccessor::readAndGet,
        "set", &LuaStatusAccessor::set,
        "setAndWrite", &LuaStatusAccessor::setAndWrite,
        "writeIfDifferent", &LuaStatusAccessor::writeIfDifferent,
        "getName", &LuaStatusAccessor::getName,
        "getUnit", &LuaStatusAccessor::getUnit,
        "getDescription", &LuaStatusAccessor::getDescription,
        "getVersionNumber", &LuaStatusAccessor::getVersionNumber,
        "isReadOnly", &LuaStatusAccessor::isReadOnly,
        "isReadable", &LuaStatusAccessor::isReadable,
        "isWriteable", &LuaStatusAccessor::isWriteable,
        "getId", &LuaStatusAccessor::getId,
        "dataValidity", &LuaStatusAccessor::dataValidity);

    // Global factory functions — owner is VariableGroup (common base for ApplicationModule and VariableGroup)
    lua.set_function("StatusOutput",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaStatusAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaStatusAccessor>(
              LuaStatusAccessor::OutputTag{}, &owner, name, description);
        });
    lua.set_function("StatusPushInput",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaStatusAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaStatusAccessor>(
              LuaStatusAccessor::PushInputTag{}, &owner, name, description);
        });
    lua.set_function("StatusPollInput",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaStatusAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaStatusAccessor>(
              LuaStatusAccessor::PollInputTag{}, &owner, name, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
