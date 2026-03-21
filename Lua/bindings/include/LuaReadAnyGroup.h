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

  class LuaReadAnyGroup {
   public:
    LuaReadAnyGroup() = default;
    explicit LuaReadAnyGroup(ReadAnyGroup&& other) : _impl(std::move(other)) {}

    void add(LuaTransferElementBase& acc) { _impl.add(acc.getTE()); }

    TransferElementID readAny() { return _impl.readAny(); }

    TransferElementID readAnyNonBlocking() { return _impl.readAnyNonBlocking(); }

    void readUntil(const TransferElementID& tid) { _impl.readUntil(tid); }

    void readUntilAccessor(LuaTransferElementBase& acc) { _impl.readUntil(acc.getTE()); }

    void finalise() { _impl.finalise(); }

    void interrupt() { _impl.interrupt(); }

    static void bind(sol::state& lua);

   private:
    ReadAnyGroup _impl;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
