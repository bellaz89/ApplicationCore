// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"
#include "ScalarAccessor.h"

#include <ChimeraTK/VariantUserTypes.h>
#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Lua wrapper for ApplicationCore scalar accessors. */
  class LuaScalarAccessor : public LuaTransferElement<LuaScalarAccessor>, public LuaOwnedObject {
    template<template<typename> class AccessorType>
    static UserTypeTemplateVariantNoVoid<ScalarAccessor> createAccessor(ChimeraTK::DataType type, Module* owner,
        const std::string& name, const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tags);

   public:
    /** Default-construct a placeholder accessor for move-based container usage. */
    LuaScalarAccessor();

    /** Construct a concrete Lua scalar accessor for the given ApplicationCore accessor type. */
    template<template<typename> class AccessorType>
    LuaScalarAccessor(AccessorTypeTag<AccessorType>, ChimeraTK::DataType type, Module* owner, const std::string& name,
        const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    LuaScalarAccessor(LuaScalarAccessor&&) = default;
    ~LuaScalarAccessor();

    /** Return the current scalar value without performing I/O. */
    sol::object get(sol::this_state s) const;

    /** Perform read() and return the resulting scalar value. */
    sol::object readAndGet(sol::this_state s);

    /** Update the wrapped scalar value without writing it out. */
    void set(sol::object val);

    /** Update the wrapped scalar value and write it immediately. */
    void setAndWrite(sol::object val);

    /** Write the given value only if it differs from the current one. */
    void writeIfDifferent(sol::object val);

    /** Register the Lua scalar-accessor bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Variant holding the concrete wrapped ApplicationCore accessor instance. */
    mutable UserTypeTemplateVariantNoVoid<ScalarAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
