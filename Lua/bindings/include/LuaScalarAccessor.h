// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"
#include "ScalarAccessor.h"

#include <ChimeraTK/VariantUserTypes.h>

namespace sol {
  class state;
  struct this_state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaScalarAccessor : public LuaTransferElement<LuaScalarAccessor>, public LuaOwnedObject {
    template<template<typename> class AccessorType>
    static UserTypeTemplateVariantNoVoid<ScalarAccessor> createAccessor(ChimeraTK::DataType type, Module* owner,
        const std::string& name, const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tags);

   public:
    LuaScalarAccessor();

    template<template<typename> class AccessorType>
    LuaScalarAccessor(AccessorTypeTag<AccessorType>, ChimeraTK::DataType type, Module* owner, const std::string& name,
        const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    LuaScalarAccessor(LuaScalarAccessor&&) = default;
    ~LuaScalarAccessor();

    sol::object get(sol::this_state s) const;
    sol::object readAndGet(sol::this_state s);
    void set(sol::object val);
    void setAndWrite(sol::object val);
    void writeIfDifferent(sol::object val);

    static void bind(sol::state& lua);

    mutable UserTypeTemplateVariantNoVoid<ScalarAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
