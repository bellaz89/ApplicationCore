// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ArrayAccessor.h"
#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"

#include <ChimeraTK/VariantUserTypes.h>
#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Lua wrapper for ApplicationCore array accessors. */
  class LuaArrayAccessor : public LuaTransferElement<LuaArrayAccessor>, public LuaOwnedObject {
    template<template<typename> class AccessorType>
    static UserTypeTemplateVariantNoVoid<ArrayAccessor> createAccessor(ChimeraTK::DataType type, Module* owner,
        const std::string& name, const std::string& unit, size_t nElements, const std::string& description,
        const std::unordered_set<std::string>& tags);

   public:
    /** Default-construct a placeholder accessor for move-based container usage. */
    LuaArrayAccessor() : _accessor(ArrayOutput<int>()) {}

    /** Construct a concrete Lua array accessor for the given ApplicationCore accessor type. */
    template<template<typename> class AccessorType>
    LuaArrayAccessor(AccessorTypeTag<AccessorType>, ChimeraTK::DataType type, Module* owner, const std::string& name,
        const std::string& unit, size_t nElements, const std::string& description,
        const std::unordered_set<std::string>& tags = {})
    : _accessor(createAccessor<AccessorType>(type, owner, name, unit, nElements, description, tags)) {}

    LuaArrayAccessor(LuaArrayAccessor&&) = default;
    ~LuaArrayAccessor();

    /** Get an element at a 1-based Lua index. */
    sol::object getElement(sol::this_state s, int index) const;

    /** Set an element at a 1-based Lua index. */
    void setElement(int index, sol::object val);

    /** Return the number of elements in the wrapped array accessor. */
    [[nodiscard]] size_t getNElements() const;

    /** Perform read() and return the accessor object for immediate Lua-side use. */
    LuaArrayAccessor& readAndGet();

    /** Register the Lua array-accessor bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Variant holding the concrete wrapped ApplicationCore accessor instance. */
    mutable UserTypeTemplateVariantNoVoid<ArrayAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
