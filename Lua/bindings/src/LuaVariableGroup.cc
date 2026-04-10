// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaVariableGroup.h"

#include "LuaArrayAccessor.h"
#include "LuaScalarAccessor.h"
#include "LuaStatusAccessor.h"
#include "LuaVoidAccessor.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaVariableGroup::bind(sol::state& lua) {
    lua.new_usertype<LuaVariableGroup>("VariableGroup",
        sol::no_constructor,
        "getName", &LuaVariableGroup::getName,
        "readAll",
        [](LuaVariableGroup& self, sol::optional<bool> includeReturnChannels) {
          self.readAll(includeReturnChannels.value_or(false));
        },
        "readAllLatest",
        [](LuaVariableGroup& self, sol::optional<bool> includeReturnChannels) {
          self.readAllLatest(includeReturnChannels.value_or(false));
        },
        "readAllNonBlocking",
        [](LuaVariableGroup& self, sol::optional<bool> includeReturnChannels) {
          self.readAllNonBlocking(includeReturnChannels.value_or(false));
        },
        "writeAll",
        [](LuaVariableGroup& self, sol::optional<bool> includeReturnChannels) {
          self.writeAll(includeReturnChannels.value_or(false));
        },
        "writeAllDestructively",
        [](LuaVariableGroup& self, sol::optional<bool> includeReturnChannels) {
          self.writeAllDestructively(includeReturnChannels.value_or(false));
        },

        // Scalar accessor factory methods (vg:ScalarPushInput(...))
        "ScalarPushInput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPushInput>{}, type, &self, name, unit,
              description);
        },
        "ScalarPushInputWB",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPushInputWB>{}, type, &self, name, unit,
              description);
        },
        "ScalarPollInput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPollInput>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutput>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutputPushRB",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutputPushRB>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutputReverseRecovery",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutputReverseRecovery>{}, type, &self, name,
              unit, description);
        },
        "ArrayPushInput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPushInput>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayPushInputWB",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPushInputWB>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayPollInput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPollInput>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayOutput",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutput>{}, type, &self, name, unit, nElements,
              description);
        },
        "ArrayOutputPushRB",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutputPushRB>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayOutputReverseRecovery",
        [](LuaVariableGroup& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutputReverseRecovery>{}, type, &self, name,
              unit, nElements, description);
        },
        "VoidInput",
        [](LuaVariableGroup& self, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidInput>{}, &self, name, description);
        },
        "VoidOutput",
        [](LuaVariableGroup& self, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidOutput>{}, &self, name, description);
        },
        "VariableGroup",
        [](LuaVariableGroup& self, const std::string& name, const std::string& description) -> LuaVariableGroup& {
          return *self.make_child<LuaVariableGroup>(&self, name, description);
        },
        "StatusOutput",
        [](LuaVariableGroup& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::OutputTag{}, &self, name, description);
        },
        "StatusPushInput",
        [](LuaVariableGroup& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::PushInputTag{}, &self, name, description);
        },
        "StatusPollInput",
        [](LuaVariableGroup& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::PollInputTag{}, &self, name, description);
        });

    // Factory function: VariableGroup(owner, name, description)
    // owner may be a LuaApplicationModule or a LuaVariableGroup — both inherit LuaOwningObject
    // via VariableGroup. We accept VariableGroup& as the common base.
    lua.set_function("VariableGroup",
        [](VariableGroup& owner, const std::string& name, const std::string& description) -> LuaVariableGroup& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaVariableGroup>(&owner, name, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
