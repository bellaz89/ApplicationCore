// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LuaOwnershipManagement.h"
#include "LuaTransferElement.h"
#include "StatusAccessor.h"

#include <variant>

namespace sol {
  class state;
  struct this_state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  class LuaStatusAccessor : public LuaTransferElement<LuaStatusAccessor>, public LuaOwnedObject {
   public:
    struct OutputTag {};
    struct PushInputTag {};
    struct PollInputTag {};

    LuaStatusAccessor();
    LuaStatusAccessor(OutputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});
    LuaStatusAccessor(PushInputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});
    LuaStatusAccessor(PollInputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    ~LuaStatusAccessor() override;

    int get() const;
    int readAndGet();
    void set(int val);
    void setAndWrite(int val);
    void writeIfDifferent(int val);

    static void bind(sol::state& lua);

    mutable std::variant<StatusOutput, StatusPushInput, StatusPollInput> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
