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

  /** Type tag used to select a concrete void-accessor flavour. */
  template<class AccessorType>
  class VoidTypeTag {};

  /********************************************************************************************************************/

  /** Lua wrapper for ApplicationCore void accessors. */
  class LuaVoidAccessor : public LuaTransferElement<LuaVoidAccessor>, public LuaOwnedObject {
   public:
    /** Default-construct a placeholder accessor for move-based container usage. */
    LuaVoidAccessor();

    /** Construct a concrete Lua void accessor for the given ApplicationCore accessor type. */
    template<class AccessorType>
    LuaVoidAccessor(VoidTypeTag<AccessorType>, Module* owner, const std::string& name,
        const std::string& description, const std::unordered_set<std::string>& tags = {});

    LuaVoidAccessor(LuaVoidAccessor&&) = default;
    ~LuaVoidAccessor() override;

    /** Register the Lua void-accessor bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Variant holding the concrete wrapped ApplicationCore accessor instance. */
    // NOLINTNEXTLINE(readability-identifier-naming)
    mutable std::variant<VoidAccessor> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
