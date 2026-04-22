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
#include <fstream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  LuaApplicationModule::LuaApplicationModule(ModuleGroup* owner, const std::string& name,
      const std::string& description, const std::string& scriptPath,
      const std::unordered_set<std::string>& tags)
  : ApplicationModule(owner, name, description, tags), _scriptPath(scriptPath) {}

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

    // Set up the global "app" reference to the parent module group (if it's a LuaModuleGroup)
    auto* parentLuaGroup = dynamic_cast<LuaModuleGroup*>(getOwner());
    if(parentLuaGroup) {
      (*_moduleState)["app"] = std::ref(*parentLuaGroup);
    }

    // Inject this module instance as a static reference so ApplicationModule() factory can return it
    // Scripts that call ApplicationModule(app, name, desc) will get this instance back
    (*_moduleState)["_modSelfRef"] = std::ref(*this);

    // Load and execute the Lua script file in this thread's state
    std::ifstream scriptFile(_scriptPath);
    if(!scriptFile.good()) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: failed to open script file '") + _scriptPath + "'");
    }

    std::string scriptContent((std::istreambuf_iterator<char>(scriptFile)),
                              std::istreambuf_iterator<char>());

    // Execute the script
    auto result = _moduleState->safe_script(scriptContent, _scriptPath, sol::script_pass_on_error);
    
    if(!result.valid()) {
      sol::error err = result;
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: error executing script '") + _scriptPath + "': " + err.what());
    }

    // Check that the script returned exactly one ApplicationModule (which should be this object)
    if(result.get_type() == sol::type::nil) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + 
          "' returned nil (must return an ApplicationModule)");
    }

    if(result.get_type() != sol::type::userdata || !result.is<LuaApplicationModule>()) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + 
          "' did not return an ApplicationModule (got " + 
          std::string(sol::type_name(_moduleState->lua_state(), result.get_type())) + ")");
    }

    auto& returnedMod = result.as<LuaApplicationModule&>();
    if(&returnedMod != this) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + 
          "' returned a different ApplicationModule instance than the one created by the framework");
    }

    // Retrieve the mainLoop function from the returned module
    sol::object mainLoopObj = result["mainLoop"];
    
    if(mainLoopObj.get_type() == sol::type::nil) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + 
          "' did not define a mainLoop function");
    }

    if(mainLoopObj.get_type() != sol::type::function) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + 
          "' mainLoop is not a function (got " + std::string(sol::type_name(_moduleState->lua_state(), mainLoopObj.get_type())) + ")");
    }

    try {
      _mainLoopFn = mainLoopObj.as<sol::protected_function>();
    }
    catch(const std::exception& e) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: failed to extract mainLoop from script '") + 
          _scriptPath + "': " + e.what());
    }

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
      // std::string_view into sol::error's internal buffer — zero allocation on the
      // common termination path (ThreadInterrupted), which is what hits this branch.
      std::string_view msg = err.what();
      if(msg.find("ChimeraTK::ThreadInterrupted") != std::string_view::npos) {
        // Normal termination via terminate() -> accessor interrupt()
        return;
      }
      if(Application::getInstance().getLifeCycleState() == LifeCycleState::shutdown &&
          msg.find("C++ exception") != std::string_view::npos) {
        return;
      }
      throw ChimeraTK::logic_error("Lua mainLoop error in '" + getName() + "': " + std::string(msg));
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::terminate() {
    ApplicationModule::terminate();
    std::lock_guard<std::mutex> lock(_mutex);
    _moduleState.reset();
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
          return *self.make_child<LuaScalarAccessor>(AccessorTypeTag<ScalarOutputReverseRecovery>{}, type, &self,
              name, unit, description);
        },
        "ArrayPushInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayPushInput>{}, type, &self, name, unit,
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
          return *self.make_child<LuaArrayAccessor>(AccessorTypeTag<ArrayOutput>{}, type, &self, name, unit,
              nElements, description);
        },
        "VoidAccessor",
        [](LuaApplicationModule& self, const std::string& name) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(&self, name);
        },
        "VariableGroup",
        [](LuaApplicationModule& self, const std::string& name) -> LuaVariableGroup& {
          return *self.make_child<LuaVariableGroup>(&self, name);
        },
        "DataConsistencyGroup",
        [](LuaApplicationModule& self) -> auto {
          return self.getDataConsistencyGroup();
        },
        "ReadAnyGroup",
        [](LuaApplicationModule& self) -> auto {
          return self.getReadAnyGroup();
        });

    // Factory function: ApplicationModule(name, description)
    // This factory function is registered in the module's lua state before script execution.
    // It returns a reference to the C++-created module instance that was injected as a global.
    // The parent group is implicit - it's already established on the C++ side.
    // 
    // We capture lua_state() at registration time so we can access the global _modSelfRef later.
    lua_State* L = lua.lua_state();
    lua.set_function("ApplicationModule",
        [L](const std::string& name, const std::string& description) -> LuaApplicationModule& {
          // Access the global _modSelfRef from the lua state where this was registered
          sol::state_view lua_view(L);
          sol::object modRef = lua_view.globals()["_modSelfRef"];
          
          if(modRef.get_type() == sol::type::nil) {
            throw ChimeraTK::logic_error(
                "ApplicationModule(): module reference not injected - internal error");
          }
          
          if(!modRef.is<LuaApplicationModule>()) {
            throw ChimeraTK::logic_error(
                "ApplicationModule(): module reference has wrong type - internal error");
          }
          
          return modRef.as<LuaApplicationModule&>();
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
