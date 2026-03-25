// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaApplicationModule.h"

#include "Application.h"
#include "LuaArrayAccessor.h"
#include "LuaBindings.h"
#include "LuaModuleGroup.h"
#include "LuaScalarAccessor.h"
#include "LuaStatusAccessor.h"
#include "LuaVariableGroup.h"
#include "LuaVoidAccessor.h"

#include <boost/thread.hpp>

#include <iostream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  // Helper: extract all upvalues from the function at funcIdx on L's stack.
  // lua_getupvalue pushes each upvalue; we pop after extracting.
  // The function at funcIdx is NOT consumed.
  static std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>>
  extractUpvaluesImpl(lua_State* L, int funcIdx);

  // Forward declaration
  static std::function<sol::object(sol::state_view)> makeFactoryImpl(sol::object val);

  /********************************************************************************************************************/

  static std::function<sol::object(sol::state_view)> makeFactoryImpl(sol::object val) {
    switch(val.get_type()) {
      case sol::type::number: {
        double v = val.as<double>();
        return [v](sol::state_view sv) { return sol::make_object(sv, v); };
      }
      case sol::type::string: {
        std::string s = val.as<std::string>();
        return [s](sol::state_view sv) { return sol::make_object(sv, s); };
      }
      case sol::type::boolean: {
        bool b = val.as<bool>();
        return [b](sol::state_view sv) { return sol::make_object(sv, b); };
      }
      case sol::type::table: {
        // Deep copy table entries into C++ factories
        sol::table tbl = val.as<sol::table>();
        using Entry = std::pair<std::function<sol::object(sol::state_view)>,
            std::function<sol::object(sol::state_view)>>;
        std::vector<Entry> entries;
        tbl.for_each([&](sol::object k, sol::object v2) {
          entries.emplace_back(makeFactoryImpl(k), makeFactoryImpl(v2));
        });
        return [entries](sol::state_view sv) {
          sol::table out = sv.create_table(static_cast<int>(entries.size()));
          for(auto& [kf, vf] : entries) {
            out[kf(sv)] = vf(sv);
          }
          return static_cast<sol::object>(out);
        };
      }
      case sol::type::userdata: {
        // C++ accessor or module — re-wrap the same pointer
        if(val.is<LuaScalarAccessor>()) {
          LuaScalarAccessor* p = &val.as<LuaScalarAccessor&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        if(val.is<LuaArrayAccessor>()) {
          LuaArrayAccessor* p = &val.as<LuaArrayAccessor&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        if(val.is<LuaVoidAccessor>()) {
          LuaVoidAccessor* p = &val.as<LuaVoidAccessor&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        if(val.is<LuaVariableGroup>()) {
          LuaVariableGroup* p = &val.as<LuaVariableGroup&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        if(val.is<LuaStatusAccessor>()) {
          LuaStatusAccessor* p = &val.as<LuaStatusAccessor&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        if(val.is<LuaApplicationModule>()) {
          LuaApplicationModule* p = &val.as<LuaApplicationModule&>();
          return [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
        }
        // Unknown userdata: store nil
        return [](sol::state_view sv) { return sol::make_object(sv, sol::lua_nil); };
      }
      case sol::type::function: {
        // Dump bytecode; extract upvalues separately
        sol::protected_function fn = val.as<sol::protected_function>();
        lua_State* L = fn.lua_state();
        fn.push();
        int funcIdx = lua_gettop(L);

        std::vector<char> bytecode;
        lua_dump(
            L,
            [](lua_State* /*L*/, const void* p, size_t sz, void* ud) -> int {
              auto* buf = static_cast<std::vector<char>*>(ud);
              const char* data = static_cast<const char*>(p);
              buf->insert(buf->end(), data, data + sz);
              return 0;
            },
            &bytecode, 0);

        // Extract upvalues while source state is alive
        auto upvalues = extractUpvaluesImpl(L, funcIdx);
        lua_pop(L, 1); // pop the function

        return [bytecode, upvalues](sol::state_view sv) -> sol::object {
          lua_State* TL = sv.lua_state();
          if(luaL_loadbuffer(TL, bytecode.data(), bytecode.size(), "migrated") != LUA_OK) {
            lua_pop(TL, 1);
            return sol::make_object(sv, sol::lua_nil);
          }
          int fi = lua_gettop(TL);
          // Patch upvalues
          for(size_t i = 0; i < upvalues.size(); ++i) {
            auto& [name, factory] = upvalues[i];
            if(name == "_ENV") continue;
            sol::object rebuilt = factory(sv);
            rebuilt.push();
            lua_setupvalue(TL, fi, static_cast<int>(i + 1));
          }
          sol::protected_function result(TL, -1);
          lua_pop(TL, 1);
          return sol::make_object(sv, result);
        };
      }
      default:
        return [](sol::state_view sv) { return sol::make_object(sv, sol::lua_nil); };
    }
  }

  /********************************************************************************************************************/

  static std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>>
  extractUpvaluesImpl(lua_State* L, int funcIdx) {
    std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>> result;
    int i = 1;
    while(true) {
      const char* name = lua_getupvalue(L, funcIdx, i);
      if(!name) break;
      std::string uvName(name);
      sol::state_view sv(L);
      sol::object upval = sol::stack::get<sol::object>(L, -1);
      lua_pop(L, 1);
      if(uvName != "_ENV") {
        result.emplace_back(uvName, makeFactoryImpl(upval));
      }
      else {
        // Keep _ENV slot as placeholder (will be auto-set by lua_load)
        result.emplace_back(uvName, [](sol::state_view sv2) { return sol::make_object(sv2, sol::lua_nil); });
      }
      ++i;
    }
    return result;
  }

  /********************************************************************************************************************/

  // Public static forwarding to the internal implementation
  std::function<sol::object(sol::state_view)> LuaApplicationModule::makeFactory(sol::object val) {
    return makeFactoryImpl(val);
  }

  /********************************************************************************************************************/

  std::vector<std::pair<std::string, std::function<sol::object(sol::state_view)>>>
  LuaApplicationModule::extractUpvalues(lua_State* L, int funcIdx) {
    return extractUpvaluesImpl(L, funcIdx);
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::captureMainLoop(sol::protected_function& fn) {
    lua_State* L = fn.lua_state();
    fn.push();
    int funcIdx = lua_gettop(L);

    // Dump bytecode
    _mainLoopBytecode.clear();
    lua_dump(
        L,
        [](lua_State* /*L*/, const void* p, size_t sz, void* ud) -> int {
          auto* buf = static_cast<std::vector<char>*>(ud);
          const char* data = static_cast<const char*>(p);
          buf->insert(buf->end(), data, data + sz);
          return 0;
        },
        &_mainLoopBytecode, 0);

    // Extract upvalues
    _mainLoopUpvalues = extractUpvaluesImpl(L, funcIdx);
    lua_pop(L, 1); // pop the function
  }

  /********************************************************************************************************************/

  LuaApplicationModule::LuaApplicationModule(ModuleGroup* owner, const std::string& name,
      const std::string& description, const std::unordered_set<std::string>& tags)
  : ApplicationModule(owner, name, description, tags) {}

  /********************************************************************************************************************/


  /********************************************************************************************************************/

  void LuaApplicationModule::run() {
    // Create the per-module Lua state. Only this module's C++ thread will access it.
    _moduleState = std::make_unique<sol::state>();

    // Register all ChimeraTK bindings in the module state
    registerLuaBindings(*_moduleState);

    // Translate boost::thread_interrupted into recognisable Lua error string
    _moduleState->set_exception_handler(
        [](lua_State* L, sol::optional<const std::exception&> /*maybeException*/,
            sol::string_view /*description*/) -> int {
          try {
            std::rethrow_exception(std::current_exception());
          }
          catch(const boost::thread_interrupted&) {
            lua_pushstring(L, "ChimeraTK::ThreadInterrupted");
            return lua_error(L);
          }
          catch(const std::exception& e) {
            lua_pushstring(L, e.what());
            return lua_error(L);
          }
          catch(...) {
            lua_pushstring(L, "unknown error");
            return lua_error(L);
          }
        });

    // Load the mainLoop bytecode into the module state.
    if(luaL_loadbuffer(_moduleState->lua_state(), _mainLoopBytecode.data(), _mainLoopBytecode.size(),
           getName().c_str()) != LUA_OK) {
      const char* err = lua_tostring(_moduleState->lua_state(), -1);
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: failed to load mainLoop bytecode for '") + getName() +
          "': " + (err ? err : "unknown error"));
    }

    int funcIdx = lua_gettop(_moduleState->lua_state());

    // Apply upvalue migration
    for(size_t i = 0; i < _mainLoopUpvalues.size(); ++i) {
      auto& [name, factory] = _mainLoopUpvalues[i];
      if(name == "_ENV") continue;
      sol::object rebuilt = factory(sol::state_view{_moduleState->lua_state()});
      rebuilt.push();
      lua_setupvalue(_moduleState->lua_state(), funcIdx, static_cast<int>(i + 1));
    }

    // The loaded function is now at funcIdx on the stack; wrap it as a protected_function.
    _mainLoopFn = sol::protected_function(_moduleState->lua_state(), funcIdx);
    lua_pop(_moduleState->lua_state(), 1);

    // Spawn the module thread via the base class.
    Application::getInstance().getTestableMode().unlock("releaseForLuaModuleStart");
    ApplicationModule::run();
    Application::getInstance().getTestableMode().lock("acquireForLuaModuleStart", false);
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::mainLoop() {
    // _mainLoopFn(this) passes this module as the "self" argument to the Lua function.
    auto result = _mainLoopFn(static_cast<LuaApplicationModule*>(this));
    if(!result.valid()) {
      sol::error err = result;
      std::string msg = err.what();
      if(msg.find("ChimeraTK::ThreadInterrupted") != std::string::npos) {
        // Normal termination via terminate() -> accessor interrupt()
        return;
      }
      throw ChimeraTK::logic_error("Lua mainLoop error in '" + getName() + "': " + msg);
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::terminate() {
    ApplicationModule::terminate();

    // Interrupt all accessible transfer elements so the Lua thread unblocks from any read()
    for(auto& var : getAccessorListRecursive()) {
      var.getAppAccessorNoType().getHighLevelImplElement()->interrupt();
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::bind(sol::state& lua) {
    lua.new_usertype<LuaApplicationModule>("ApplicationModule",
        sol::no_constructor,
        "getName", &LuaApplicationModule::getName,
        "readAll",
        [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAll(includeReturnChannels.value_or(false));
        },
        "readAllLatest",
        [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAllLatest(includeReturnChannels.value_or(false));
        },
        "readAllNonBlocking",
        [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAllNonBlocking(includeReturnChannels.value_or(false));
        },
        "writeAll",
        [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.writeAll(includeReturnChannels.value_or(false));
        },
        "getCurrentVersionNumber", &LuaApplicationModule::getCurrentVersionNumber,
        "setCurrentVersionNumber", &LuaApplicationModule::setCurrentVersionNumber,
        "getDataValidity", &LuaApplicationModule::getDataValidity,
        "incrementDataFaultCounter", &LuaApplicationModule::incrementDataFaultCounter,
        "decrementDataFaultCounter", &LuaApplicationModule::decrementDataFaultCounter,
        "getDataFaultCounter", &LuaApplicationModule::getDataFaultCounter,
        "disable", &LuaApplicationModule::disable,

        // Scalar accessor factory methods (mod:ScalarPushInput(...))
        "ScalarPushInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPushInput>{}, type, &self, name, unit,
              description);
        },
        "ScalarPushInputWB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPushInputWB>{}, type, &self, name, unit,
              description);
        },
        "ScalarPollInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarPollInput>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutput>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutputPushRB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutputPushRB>{}, type, &self, name, unit,
              description);
        },
        "ScalarOutputReverseRecovery",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutputReverseRecovery>{}, type, &self, name,
              unit, description);
        },
        "ArrayPushInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPushInput>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayPushInputWB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPushInputWB>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayPollInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPollInput>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayOutput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutput>{}, type, &self, name, unit, nElements,
              description);
        },
        "ArrayOutputPushRB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutputPushRB>{}, type, &self, name, unit,
              nElements, description);
        },
        "ArrayOutputReverseRecovery",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutputReverseRecovery>{}, type, &self, name,
              unit, nElements, description);
        },
        "VoidInput",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidInput>{}, &self, name, description);
        },
        "VoidOutput",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidOutput>{}, &self, name, description);
        },
        "VariableGroup",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaVariableGroup& {
          return *self.make_child<LuaVariableGroup>(&self, name, description);
        },
        "StatusOutput",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::OutputTag{}, &self, name, description);
        },
        "StatusPushInput",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::PushInputTag{}, &self, name, description);
        },
        "StatusPollInput",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaStatusAccessor& {
          return *self.make_child<LuaStatusAccessor>(LuaStatusAccessor::PollInputTag{}, &self, name, description);
        },

        // __newindex: capture mainLoop or store all other values as factories
        sol::meta_function::new_index,
        [](LuaApplicationModule& self, const std::string& key, sol::object val) {
          // Handle mainLoop specially: capture bytecode + upvalues
          if(key == "mainLoop" && val.get_type() == sol::type::function) {
            sol::protected_function fn = val.as<sol::protected_function>();
            self.captureMainLoop(fn);
            return;
          }
          // Store all other properties as factories (accessors, groups, plain values)
          auto factory = LuaApplicationModule::makeFactory(val);
          self._properties[key] = std::move(factory);
        },

        // __index fallback: look up in _properties after named-method lookup misses
        sol::meta_function::index,
        [](LuaApplicationModule& self, const std::string& key, sol::this_state s) -> sol::object {
          auto it = self._properties.find(key);
          if(it != self._properties.end()) {
            return it->second(sol::state_view{s});
          }
          return sol::make_object(sol::state_view{s}, sol::lua_nil);
        });

    // Factory function: ApplicationModule(owner, name, description)
    lua.set_function("ApplicationModule",
        [](LuaModuleGroup& owner, const std::string& name, const std::string& description) -> LuaApplicationModule& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaApplicationModule>(&owner, name, description);
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
