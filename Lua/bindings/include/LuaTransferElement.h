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

  /// Base class for LuaScalarAccessor, LuaArrayAccessor and LuaVoidAccessor.
  /// Provides access to the underlying TransferElementAbstractor used by LuaReadAnyGroup.
  class LuaTransferElementBase {
   public:
    virtual ~LuaTransferElementBase() = default;

    virtual const TransferElementAbstractor& getTE() const = 0;
    virtual TransferElementAbstractor& getTE() = 0;

    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

  /// CRTP template base providing I/O forwarding for all accessor types.
  ///
  /// No mutex is used here. Each LuaApplicationModule runs its mainLoop in its own lua_State accessed from exactly
  /// one C++ thread, so no Lua-level locking is needed. The underlying C++ ApplicationCore operations (read, write,
  /// etc.) are already thread-safe.
  template<class DerivedAccessor>
  class LuaTransferElement : public LuaTransferElementBase {
   public:
    void read() { visit([](auto& acc) { acc.read(); }); }

    bool readNonBlocking() { return visit([](auto& acc) { return acc.readNonBlocking(); }); }

    bool readLatest() { return visit([](auto& acc) { return acc.readLatest(); }); }

    void write() { visit([](auto& acc) { acc.write(); }); }

    void writeDestructively() { visit([](auto& acc) { acc.writeDestructively(); }); }

    auto getName() const { return getTE().getName(); }
    auto getUnit() const { return getTE().getUnit(); }
    auto getDescription() const { return getTE().getDescription(); }
    DataType getValueType() const { return getTE().getValueType(); }
    auto getVersionNumber() const { return getTE().getVersionNumber(); }
    auto isReadOnly() const { return getTE().isReadOnly(); }
    auto isReadable() const { return getTE().isReadable(); }
    auto isWriteable() const { return getTE().isWriteable(); }
    auto getId() const { return getTE().getId(); }
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

  template<template<typename> class AccessorType>
  class AccessorTypeTag {};

  /********************************************************************************************************************/

} // namespace ChimeraTK
