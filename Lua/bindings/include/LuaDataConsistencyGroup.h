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

  class LuaDataConsistencyGroup {
   public:
    LuaDataConsistencyGroup() = default;
    explicit LuaDataConsistencyGroup(DataConsistencyGroup::MatchingMode mode) : _impl(mode) {}
    explicit LuaDataConsistencyGroup(DataConsistencyGroup&& other) : _impl(std::move(other)) {}

    void add(LuaTransferElementBase& acc, unsigned histLen = DataConsistencyGroup::defaultHistLen) {
      _impl.add(acc.getTE(), histLen);
    }

    bool update(const TransferElementID& tid) { return _impl.update(tid); }

    [[nodiscard]] DataConsistencyGroup::MatchingMode getMatchingMode() const { return _impl.getMatchingMode(); }

    static void bind(sol::state& lua);

   private:
    DataConsistencyGroup _impl;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
