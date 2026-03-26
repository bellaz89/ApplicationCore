// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaTransferElement.h"

#include <ChimeraTK/DataConsistencyGroup.h>

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Lua wrapper for DataConsistencyGroup. */
  class LuaDataConsistencyGroup {
   public:
    LuaDataConsistencyGroup() = default;
    /** Construct a data-consistency group with the given matching mode. */
    explicit LuaDataConsistencyGroup(DataConsistencyGroup::MatchingMode mode) : _impl(mode) {}
    /** Wrap an existing DataConsistencyGroup instance. */
    explicit LuaDataConsistencyGroup(DataConsistencyGroup&& other) : _impl(std::move(other)) {}

    /** Add an accessor to the group with the given history length. */
    void add(LuaTransferElementBase& acc, unsigned histLen = DataConsistencyGroup::defaultHistLen) {
      _impl.add(acc.getTE(), histLen);
    }

    /** Update the group state with the given transfer-element ID. */
    bool update(const TransferElementID& tid) { return _impl.update(tid); }

    /** Return the configured matching mode. */
    [[nodiscard]] DataConsistencyGroup::MatchingMode getMatchingMode() const { return _impl.getMatchingMode(); }

    /** Register the Lua DataConsistencyGroup bindings into the given Lua state. */
    static void bind(sol::state& lua);

   private:
    DataConsistencyGroup _impl;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
