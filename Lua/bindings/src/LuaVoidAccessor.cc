// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaVoidAccessor.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  LuaVoidAccessor::LuaVoidAccessor() : _accessor(VoidOutput()) {}

  /********************************************************************************************************************/

  template<class AccessorType>
  LuaVoidAccessor::LuaVoidAccessor(VoidTypeTag<AccessorType>, Module* owner, const std::string& name,
      const std::string& description, const std::unordered_set<std::string>& tags)
  : _accessor(std::move(AccessorType(owner, name, description, tags))) {}

  /********************************************************************************************************************/

  LuaVoidAccessor::~LuaVoidAccessor() = default;

  /********************************************************************************************************************/

  void LuaVoidAccessor::bind(sol::state& lua) {
    lua.new_usertype<LuaVoidAccessor>("VoidAccessor",
        sol::no_constructor,
        sol::base_classes, sol::bases<LuaTransferElementBase>(),
        "read", &LuaVoidAccessor::read,
        "readNonBlocking", &LuaVoidAccessor::readNonBlocking,
        "readLatest", &LuaVoidAccessor::readLatest,
        "write", &LuaVoidAccessor::write,
        "writeDestructively", &LuaVoidAccessor::writeDestructively,
        "getName", &LuaVoidAccessor::getName,
        "getUnit", &LuaVoidAccessor::getUnit,
        "getDescription", &LuaVoidAccessor::getDescription,
        "getVersionNumber", &LuaVoidAccessor::getVersionNumber,
        "isReadOnly", &LuaVoidAccessor::isReadOnly,
        "isReadable", &LuaVoidAccessor::isReadable,
        "isWriteable", &LuaVoidAccessor::isWriteable,
        "getId", &LuaVoidAccessor::getId,
        "dataValidity", &LuaVoidAccessor::dataValidity);

    lua.set_function("VoidInput",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaVoidAccessor>(
              VoidTypeTag<VoidInput>{}, &owner, name, description);
        });
    lua.set_function("VoidOutput",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaVoidAccessor>(
              VoidTypeTag<VoidOutput>{}, &owner, name, description);
        });
  }

  /********************************************************************************************************************/


  template LuaVoidAccessor::LuaVoidAccessor(VoidTypeTag<VoidInput>, Module*, const std::string&, const std::string&, const std::unordered_set<std::string>&);
  template LuaVoidAccessor::LuaVoidAccessor(VoidTypeTag<VoidOutput>, Module*, const std::string&, const std::string&, const std::unordered_set<std::string>&);

} // namespace ChimeraTK
