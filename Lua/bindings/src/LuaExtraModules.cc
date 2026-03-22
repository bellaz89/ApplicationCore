// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaExtraModules.h"

#include "LuaModuleGroup.h"
#include "LuaOwnershipManagement.h"
#include "PeriodicTrigger.h"
#include "StatusAggregator.h"
#include "StatusMonitor.h"

#include <ChimeraTK/SupportedUserTypes.h>

#include <sol/sol.hpp>

#include <list>
#include <memory>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /// A generic LuaOwnedObject that wraps an ApplicationModule-derived type.
  /// Keeps the module alive for the lifetime of the owning LuaOwningObject.
  template<typename T>
  struct LuaOwnedApplicationModule : LuaOwnedObject {
    template<typename... Args>
    explicit LuaOwnedApplicationModule(Args&&... args) : module(std::forward<Args>(args)...) {}
    T module;
  };

  /********************************************************************************************************************/

  void LuaExtraModules::bind(sol::state& lua) {
    // PeriodicTrigger
    lua.set_function("PeriodicTrigger",
        sol::overload(
            [](LuaModuleGroup& owner, const std::string& name, const std::string& desc,
                uint32_t defaultPeriod) {
              dynamic_cast<LuaOwningObject&>(owner)
                  .make_child<LuaOwnedApplicationModule<PeriodicTrigger>>(&owner, name, desc, defaultPeriod);
            },
            [](LuaModuleGroup& owner, const std::string& name, const std::string& desc) {
              dynamic_cast<LuaOwningObject&>(owner)
                  .make_child<LuaOwnedApplicationModule<PeriodicTrigger>>(&owner, name, desc);
            }));

    // PriorityMode enum for StatusAggregator
    lua["PriorityMode"] = lua.create_table_with("fwok", StatusAggregator::PriorityMode::fwok, "fwko",
        StatusAggregator::PriorityMode::fwko, "fw_warn_mixed", StatusAggregator::PriorityMode::fw_warn_mixed, "ofwk",
        StatusAggregator::PriorityMode::ofwk);

    // StatusAggregator
    lua.set_function("StatusAggregator",
        sol::overload(
            [](LuaModuleGroup& owner, const std::string& outputName, const std::string& desc,
                StatusAggregator::PriorityMode mode) {
              dynamic_cast<LuaOwningObject&>(owner)
                  .make_child<LuaOwnedApplicationModule<StatusAggregator>>(&owner, outputName, desc, mode);
            },
            [](LuaModuleGroup& owner, const std::string& outputName, const std::string& desc) {
              dynamic_cast<LuaOwningObject&>(owner)
                  .make_child<LuaOwnedApplicationModule<StatusAggregator>>(&owner, outputName, desc);
            }));

    // StatusMonitor variants — fire-and-forget, no return value
    lua.set_function("MaxMonitor",
        [](ChimeraTK::DataType type, LuaModuleGroup& owner, const std::string& inputPath,
            const std::string& outputPath, const std::string& parameterPath, const std::string& desc) {
          ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
            using T = decltype(t);
            dynamic_cast<LuaOwningObject&>(owner)
                .make_child<LuaOwnedApplicationModule<MaxMonitor<T>>>(&owner, inputPath, outputPath, parameterPath,
                    desc);
          });
        });
    lua.set_function("MinMonitor",
        [](ChimeraTK::DataType type, LuaModuleGroup& owner, const std::string& inputPath,
            const std::string& outputPath, const std::string& parameterPath, const std::string& desc) {
          ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
            using T = decltype(t);
            dynamic_cast<LuaOwningObject&>(owner)
                .make_child<LuaOwnedApplicationModule<MinMonitor<T>>>(&owner, inputPath, outputPath, parameterPath,
                    desc);
          });
        });
    lua.set_function("RangeMonitor",
        [](ChimeraTK::DataType type, LuaModuleGroup& owner, const std::string& inputPath,
            const std::string& outputPath, const std::string& parameterPath, const std::string& desc) {
          ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
            using T = decltype(t);
            dynamic_cast<LuaOwningObject&>(owner)
                .make_child<LuaOwnedApplicationModule<RangeMonitor<T>>>(&owner, inputPath, outputPath, parameterPath,
                    desc);
          });
        });
    lua.set_function("ExactMonitor",
        [](ChimeraTK::DataType type, LuaModuleGroup& owner, const std::string& inputPath,
            const std::string& outputPath, const std::string& parameterPath, const std::string& desc) {
          ChimeraTK::callForTypeNoVoid(type, [&](auto t) {
            using T = decltype(t);
            dynamic_cast<LuaOwningObject&>(owner)
                .make_child<LuaOwnedApplicationModule<ExactMonitor<T>>>(&owner, inputPath, outputPath, parameterPath,
                    desc);
          });
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
