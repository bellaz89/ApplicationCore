// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ArrayAccessor.h"
#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"

#include <ChimeraTK/VariantUserTypes.h>

namespace sol {
  class state;
  struct this_state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaArrayAccessor : public LuaTransferElement<LuaArrayAccessor>, public LuaOwnedObject {
    template<template<typename> class AccessorType>
    static UserTypeTemplateVariantNoVoid<ArrayAccessor> createAccessor(ChimeraTK::DataType type, Module* owner,
        const std::string& name, const std::string& unit, size_t nElements, const std::string& description,
        const std::unordered_set<std::string>& tags);

   public:
    LuaArrayAccessor() : _accessor(ArrayOutput<int>()) {}

    template<template<typename> class AccessorType>
    LuaArrayAccessor(AccessorTypeTag<AccessorType>, ChimeraTK::DataType type, Module* owner, const std::string& name,
        const std::string& unit, size_t nElements, const std::string& description,
        const std::unordered_set<std::string>& tags = {})
    : _accessor(createAccessor<AccessorType>(type, owner, name, unit, nElements, description, tags)) {}

    LuaArrayAccessor(LuaArrayAccessor&&) = default;
    ~LuaArrayAccessor();

    /// Get element at 1-based Lua index — view into the underlying C++ buffer (no copy).
    sol::object getElement(sol::this_state s, int index) const;

    /// Set element at 1-based Lua index directly into the underlying C++ buffer (no copy).
    void setElement(int index, sol::object val);

    [[nodiscard]] size_t getNElements() const;

    /// Convenience: read() then return self so the caller can iterate immediately.
    /// e.g.: for i, v in pairs(arr:read()) do ... end
    LuaArrayAccessor& readAndGet();

    static void bind(sol::state& lua);

    mutable UserTypeTemplateVariantNoVoid<ArrayAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
