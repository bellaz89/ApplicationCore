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

    /**
     * Copy all elements into a fresh Lua table (1-based) in one std::visit dispatch.
     *
     * Prefer this over element-by-element arr[i] access in hot loops: the per-element
     * path pays one std::visit + one sol::make_object per index, whereas toTable pays
     * that overhead exactly once regardless of array length.
     */
    sol::table toTable(sol::this_state s) const;

    /**
     * Fill a pre-existing 1-based Lua table in-place with the first maxN elements
     * (defaulting to getNElements() when maxN is absent).  The table is grown as needed.
     * Returns nothing; the caller keeps its reference to t.
     *
     * Prefer this over toTable() in hot loops: it avoids allocating a new Lua table
     * on every call and therefore reduces GC pressure at high DAQ rates.
     */
    void fillTable(sol::table t, std::optional<size_t> maxN = std::nullopt) const;

    /**
     * Bulk-write all elements from a 1-based Lua table into the array buffer.
     *
     * Symmetric counterpart to toTable: a single std::visit replaces N individual
     * setElement calls, each of which would dispatch through the variant separately.
     * Only elements [1..getNElements()] are read from the table.
     */
    void fromTable(sol::table t);

    /** Register the Lua array-accessor bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Variant holding the concrete wrapped ApplicationCore accessor instance. */
    mutable UserTypeTemplateVariantNoVoid<ArrayAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
