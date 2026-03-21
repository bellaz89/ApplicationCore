// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"
#include "VoidAccessor.h"

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<class AccessorType>
  class VoidTypeTag {};

  /********************************************************************************************************************/

  class LuaVoidAccessor : public LuaTransferElement<LuaVoidAccessor>, public LuaOwnedObject {
   public:
    LuaVoidAccessor();

    template<class AccessorType>
    LuaVoidAccessor(VoidTypeTag<AccessorType>, Module* owner, const std::string& name,
        const std::string& description, const std::unordered_set<std::string>& tags = {});

    LuaVoidAccessor(LuaVoidAccessor&&) = default;
    ~LuaVoidAccessor() override;

    static void bind(sol::state& lua);

    // NOLINTNEXTLINE(readability-identifier-naming)
    mutable std::variant<VoidAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
