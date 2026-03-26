// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaTransferElement.h"

#include <ChimeraTK/ReadAnyGroup.h>

namespace sol {
  class state;
  struct variadic_args;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Lua wrapper for ReadAnyGroup. */
  class LuaReadAnyGroup {
   public:
    LuaReadAnyGroup() = default;
    /** Wrap an existing ReadAnyGroup instance. */
    explicit LuaReadAnyGroup(ReadAnyGroup&& other) : _impl(std::move(other)) {}

    /** Add an accessor to the group. */
    void add(LuaTransferElementBase& acc) { _impl.add(acc.getTE()); }

    /** Wait for any member accessor to update and return its ID. */
    TransferElementID readAny() { return _impl.readAny(); }

    /** Perform a non-blocking wait for any member accessor update. */
    TransferElementID readAnyNonBlocking() { return _impl.readAnyNonBlocking(); }

    /** Read until the given transfer-element ID is observed. */
    void readUntil(const TransferElementID& tid) { _impl.readUntil(tid); }

    /** Read until the given accessor is observed. */
    void readUntilAccessor(LuaTransferElementBase& acc) { _impl.readUntil(acc.getTE()); }

    /** Finalise the group after all members have been added. */
    void finalise() { _impl.finalise(); }

    /** Interrupt any blocking wait in the underlying group. */
    void interrupt() { _impl.interrupt(); }

    /** Register the Lua ReadAnyGroup bindings into the given Lua state. */
    static void bind(sol::state& lua);

   private:
    ReadAnyGroup _impl;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
