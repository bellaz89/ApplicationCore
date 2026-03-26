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

  /** Lua wrapper for ApplicationCore status accessors. */
  class LuaStatusAccessor : public LuaTransferElement<LuaStatusAccessor>, public LuaOwnedObject {
   public:
    /** Tag selecting creation of a StatusOutput accessor. */
    struct OutputTag {};
    /** Tag selecting creation of a StatusPushInput accessor. */
    struct PushInputTag {};
    /** Tag selecting creation of a StatusPollInput accessor. */
    struct PollInputTag {};

    /** Default-construct a placeholder accessor for move-based container usage. */
    LuaStatusAccessor();
    /** Construct a Lua wrapper around a StatusOutput accessor. */
    LuaStatusAccessor(OutputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});
    /** Construct a Lua wrapper around a StatusPushInput accessor. */
    LuaStatusAccessor(PushInputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});
    /** Construct a Lua wrapper around a StatusPollInput accessor. */
    LuaStatusAccessor(PollInputTag, Module* owner, const std::string& name, const std::string& description,
        const std::unordered_set<std::string>& tags = {});

    ~LuaStatusAccessor() override;

    /** Return the current status value without performing I/O. */
    int get() const;
    /** Perform read() and return the resulting status value. */
    int readAndGet();
    /** Update the wrapped status value without writing it out. */
    void set(int val);
    /** Update the wrapped status value and write it immediately. */
    void setAndWrite(int val);
    /** Write the given status only if it differs from the current one. */
    void writeIfDifferent(int val);

    /** Register the Lua status-accessor bindings into the given Lua state. */
    static void bind(sol::state& lua);

    /** Variant holding the concrete wrapped ApplicationCore status accessor instance. */
    mutable std::variant<StatusOutput, StatusPushInput, StatusPollInput> _accessor;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
