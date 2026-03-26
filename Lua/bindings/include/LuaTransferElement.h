// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/TransferElementAbstractor.h>

// Forward-declare sol::state to avoid pulling in the full sol2 header here
namespace sol {
  class state;
}

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Base class for LuaScalarAccessor, LuaArrayAccessor, LuaStatusAccessor and LuaVoidAccessor.
   *
   * Provides access to the underlying TransferElementAbstractor used by helper
   * types such as LuaReadAnyGroup and LuaDataConsistencyGroup.
   */
  class LuaTransferElementBase {
   public:
    virtual ~LuaTransferElementBase() = default;

    /** Return the underlying transfer element as const reference. */
    virtual const TransferElementAbstractor& getTE() const = 0;

    /** Return the underlying transfer element as mutable reference. */
    virtual TransferElementAbstractor& getTE() = 0;

    /** Register the Lua transfer-element base type in the given Lua state. */
    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

  /**
   * CRTP template base providing I/O forwarding for all Lua accessor types.
   *
   * No mutex is used here. Each LuaApplicationModule runs its mainLoop in its
   * own lua_State accessed from exactly one C++ thread, so no Lua-level
   * locking is needed. The underlying C++ ApplicationCore operations are
   * already thread-safe.
   */
  template<class DerivedAccessor>
  class LuaTransferElement : public LuaTransferElementBase {
   public:
    /** Forward a blocking read to the wrapped accessor. */
    void read() { visit([](auto& acc) { acc.read(); }); }

    /** Forward a non-blocking read to the wrapped accessor. */
    bool readNonBlocking() { return visit([](auto& acc) { return acc.readNonBlocking(); }); }

    /** Forward a read-latest operation to the wrapped accessor. */
    bool readLatest() { return visit([](auto& acc) { return acc.readLatest(); }); }

    /** Forward a write to the wrapped accessor. */
    void write() { visit([](auto& acc) { acc.write(); }); }

    /** Forward a destructive write to the wrapped accessor. */
    void writeDestructively() { visit([](auto& acc) { acc.writeDestructively(); }); }

    /** Return the accessor name. */
    auto getName() const { return getTE().getName(); }

    /** Return the engineering unit. */
    auto getUnit() const { return getTE().getUnit(); }

    /** Return the accessor description. */
    auto getDescription() const { return getTE().getDescription(); }

    /** Return the accessor value type. */
    DataType getValueType() const { return getTE().getValueType(); }

    /** Return the current version number. */
    auto getVersionNumber() const { return getTE().getVersionNumber(); }

    /** Return whether the wrapped accessor is read-only. */
    auto isReadOnly() const { return getTE().isReadOnly(); }

    /** Return whether the wrapped accessor can be read. */
    auto isReadable() const { return getTE().isReadable(); }

    /** Return whether the wrapped accessor can be written. */
    auto isWriteable() const { return getTE().isWriteable(); }

    /** Return the transfer-element ID. */
    auto getId() const { return getTE().getId(); }

    /** Return the current data validity. */
    auto dataValidity() const { return getTE().dataValidity(); }

    [[nodiscard]] const TransferElementAbstractor& getTE() const final {
      const auto* self = static_cast<const DerivedAccessor*>(this);
      TransferElementAbstractor* te{nullptr};
      std::visit([&](auto& acc) { te = &acc; }, self->_accessor);
      return *te;
    }

    [[nodiscard]] TransferElementAbstractor& getTE() final {
      auto* self = static_cast<DerivedAccessor*>(this);
      TransferElementAbstractor* te{nullptr};
      std::visit([&](auto& acc) { te = &acc; }, self->_accessor);
      return *te;
    }

    template<typename CALLABLE>
    auto visit(CALLABLE fn) const {
      const auto* self = static_cast<const DerivedAccessor*>(this);
      return std::visit(fn, self->_accessor);
    }

    template<typename CALLABLE>
    auto visit(CALLABLE fn) {
      auto* self = static_cast<DerivedAccessor*>(this);
      return std::visit(fn, self->_accessor);
    }
  };

  /********************************************************************************************************************/

  /** Type tag used to select the concrete ApplicationCore accessor type from Lua bindings. */
  template<template<typename> class AccessorType>
  class AccessorTypeTag {};

  /********************************************************************************************************************/

} // namespace ChimeraTK
