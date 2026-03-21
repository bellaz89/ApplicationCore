// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaApplicationModule.h"

#include "Application.h"
#include "LuaArrayAccessor.h"
#include "LuaBindings.h"
#include "LuaScalarAccessor.h"
#include "LuaVariableGroup.h"
#include "LuaVoidAccessor.h"

#include <boost/thread.hpp>

#include <iostream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  LuaApplicationModule::LuaApplicationModule(ModuleGroup* owner, const std::string& name,
      const std::string& description, sol::protected_function mainLoopFn,
      const std::unordered_set<std::string>& tags)
  : ApplicationModule(owner, name, description, tags) {
    // Dump the mainLoop Lua function to bytecode so it can be loaded into the per-module lua_State later.
    // This must be done while the function is still alive in the loading state.
    lua_State* L = mainLoopFn.lua_state();

    // Push the function onto the Lua stack
    mainLoopFn.push();

    // lua_dump writes the function at stack top to a byte buffer
    lua_dump(
        L,
        [](lua_State* /*L*/, const void* p, size_t sz, void* ud) -> int {
          auto* buf = static_cast<std::vector<char>*>(ud);
          const auto* data = static_cast<const char*>(p);
          buf->insert(buf->end(), data, data + sz);
          return 0;
        },
        &_mainLoopBytecode, 0);

    lua_pop(L, 1);
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::run() {
    // Create the per-module Lua state. Only this module's C++ thread will access it.
    _moduleState = std::make_unique<sol::state>();

    // Register all ChimeraTK bindings in the module state
    registerLuaBindings(*_moduleState);

    // Translate boost::thread_interrupted (thrown by accessor interrupt()) into a
    // recognisable Lua error string so mainLoop() can distinguish normal termination
    // from real errors without depending on a specific error message format.
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
    // luaL_loadbuffer detects bytecode (starts with \x1b) and loads it directly.
    // The function's _ENV upvalue is automatically set to the new state's _G.
    if(luaL_loadbuffer(_moduleState->lua_state(), _mainLoopBytecode.data(), _mainLoopBytecode.size(),
           getName().c_str()) != LUA_OK) {
      const char* err = lua_tostring(_moduleState->lua_state(), -1);
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: failed to load mainLoop bytecode for '") + getName() +
          "': " + (err ? err : "unknown error"));
    }

    // The loaded function is now on top of the stack; wrap it as a protected_function.
    _mainLoopFn = sol::protected_function(_moduleState->lua_state(), -1);
    lua_pop(_moduleState->lua_state(), 1);

    // Spawn the module thread via the base class.
    // ApplicationModule::run() starts a boost::thread running mainLoopWrapper(),
    // which calls prepare() then mainLoop() (our override below).
    Application::getInstance().getTestableMode().unlock("releaseForLuaModuleStart");
    ApplicationModule::run();
    Application::getInstance().getTestableMode().lock("acquireForLuaModuleStart", false);
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::mainLoop() {
    // Called from the module thread inside mainLoopWrapper().
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
        "readAll", [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAll(includeReturnChannels.value_or(false));
        },
        "readAllLatest", [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAllLatest(includeReturnChannels.value_or(false));
        },
        "readAllNonBlocking", [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.readAllNonBlocking(includeReturnChannels.value_or(false));
        },
        "writeAll", [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.writeAll(includeReturnChannels.value_or(false));
        },
        "getCurrentVersionNumber", &LuaApplicationModule::getCurrentVersionNumber,
        "setCurrentVersionNumber", &LuaApplicationModule::setCurrentVersionNumber,
        "getDataValidity", &LuaApplicationModule::getDataValidity,
        "incrementDataFaultCounter", &LuaApplicationModule::incrementDataFaultCounter,
        "decrementDataFaultCounter", &LuaApplicationModule::decrementDataFaultCounter,
        "getDataFaultCounter", &LuaApplicationModule::getDataFaultCounter,
        "disable", &LuaApplicationModule::disable,

        // __newindex: store named accessors/groups in the C++ map so they survive VM migration.
        sol::meta_function::new_index,
        [](LuaApplicationModule& self, const std::string& key, sol::object val) {
          // Dispatch on all registered accessor/group types and store typed factory functions.
          // The factory is called from the module VM to produce a properly-typed Lua userdata.
          if(val.is<LuaScalarAccessor>()) {
            auto* p = &val.as<LuaScalarAccessor&>();
            self._namedAccessors[key] = [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
          }
          else if(val.is<LuaArrayAccessor>()) {
            auto* p = &val.as<LuaArrayAccessor&>();
            self._namedAccessors[key] = [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
          }
          else if(val.is<LuaVoidAccessor>()) {
            auto* p = &val.as<LuaVoidAccessor&>();
            self._namedAccessors[key] = [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
          }
          else if(val.is<LuaVariableGroup>()) {
            auto* p = &val.as<LuaVariableGroup&>();
            self._namedAccessors[key] = [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
          }
          else if(val.is<LuaApplicationModule>()) {
            auto* p = &val.as<LuaApplicationModule&>();
            self._namedAccessors[key] = [p](sol::state_view sv) { return sol::make_object(sv, std::ref(*p)); };
          }
          // Non-accessor values are ignored (use local Lua variables instead).
        },

        // __index fallback: look up in _namedAccessors after method lookup misses.
        sol::meta_function::index,
        [](LuaApplicationModule& self, const std::string& key, sol::this_state s) -> sol::object {
          auto it = self._namedAccessors.find(key);
          if(it != self._namedAccessors.end()) {
            return it->second(sol::state_view{s});
          }
          return sol::lua_nil;
        });

    // Factory function: ApplicationModule(owner, name, description, mainLoopFn)
    lua.set_function("ApplicationModule",
        [](LuaModuleGroup& owner, const std::string& name, const std::string& description,
            sol::protected_function mainLoopFn) -> LuaApplicationModule& {
          return *dynamic_cast<LuaOwningObject&>(owner).make_child<LuaApplicationModule>(
              &owner, name, description, std::move(mainLoopFn));
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
