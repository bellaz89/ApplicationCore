// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaApplicationModule.h"

#include "Application.h"
#include "LuaArrayAccessor.h"
#include "LuaBindings.h"
#include "LuaScalarAccessor.h"
#include "LuaStatusAccessor.h"
#include "LuaVariableGroup.h"
#include "LuaVoidAccessor.h"

#include <boost/thread.hpp>

#include <fstream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  LuaApplicationModule::LuaApplicationModule(ModuleGroup* owner, const std::string& name,
      const std::string& description, const std::string& scriptPath)
  : ApplicationModule(owner, name, description), _scriptPath(scriptPath) {
    // Script execution must happen at construction time: the accessors the script creates
    // register with this module, and the framework needs them to be present before
    // runApplication() / initialise() wires the control-system PV manager. Running the
    // script in run() (i.e. at thread-start time) would be too late — TestFacility::getScalar
    // would fail to find PVs that the script has not yet created.
    loadScript();
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::loadScript() {
    _moduleState = std::make_unique<sol::state>();
    registerLuaBindings(*_moduleState);

    // Translate boost::thread_interrupted into a recognisable Lua error string so mainLoop() can
    // distinguish a normal shutdown interrupt from a genuine script error.
    _moduleState->set_exception_handler([](lua_State* L, sol::optional<const std::exception&> /*maybeException*/,
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

    // The Lua ApplicationModule(name, desc) factory returns this pre-existing C++ instance rather
    // than constructing a new one; it reads _modSelfRef from the global table.
    (*_moduleState)["_modSelfRef"] = std::ref(*this);

    std::ifstream scriptFile(_scriptPath);
    if(!scriptFile.good()) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: failed to open script file '") + _scriptPath + "'");
    }

    std::string scriptContent((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());

    // safe_script signature: (code, on_error, chunkname).
    auto result = _moduleState->safe_script(scriptContent, sol::script_pass_on_error, _scriptPath);

    if(!result.valid()) {
      sol::error err = result;
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: error executing script '") + _scriptPath + "': " + err.what());
    }

    // protected_function_result exposes a narrow API in this sol2 version; go through sol::object.
    sol::object ret = result.get<sol::object>();

    if(ret.get_type() == sol::type::nil) {
      throw ChimeraTK::logic_error(std::string("LuaApplicationModule: script '") + _scriptPath +
          "' returned nil (must return an ApplicationModule)");
    }

    if(ret.get_type() != sol::type::userdata || !ret.is<LuaApplicationModule>()) {
      throw ChimeraTK::logic_error(std::string("LuaApplicationModule: script '") + _scriptPath +
          "' did not return an ApplicationModule (got " +
          std::string(sol::type_name(_moduleState->lua_state(), ret.get_type())) + ")");
    }

    if(&ret.as<LuaApplicationModule&>() != this) {
      throw ChimeraTK::logic_error(std::string("LuaApplicationModule: script '") + _scriptPath +
          "' returned a different ApplicationModule instance than the one created by the framework");
    }

    // Fields set via `function mod:mainLoop()` land in the userdata's __newindex. Going through
    // sol::table here dispatches through the usertype's __index to retrieve them.
    sol::object mainLoopObj = ret.as<sol::table>()["mainLoop"];

    if(mainLoopObj.get_type() == sol::type::nil) {
      throw ChimeraTK::logic_error(
          std::string("LuaApplicationModule: script '") + _scriptPath + "' did not define a mainLoop function");
    }

    if(mainLoopObj.get_type() != sol::type::function) {
      throw ChimeraTK::logic_error(std::string("LuaApplicationModule: script '") + _scriptPath +
          "' mainLoop is not a function (got " +
          std::string(sol::type_name(_moduleState->lua_state(), mainLoopObj.get_type())) + ")");
    }

    _mainLoopFn = mainLoopObj.as<sol::protected_function>();

    // prepare is optional: scripts that need to seed initial values for push-input consumers
    // can define `function mod:prepare()`; scripts that don't need it can omit it.
    sol::object prepareObj = ret.as<sol::table>()["prepare"];
    if(prepareObj.valid() && prepareObj.get_type() == sol::type::function) {
      _prepareFn = prepareObj.as<sol::protected_function>();
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::prepare() {
    if(!_prepareFn.valid()) {
      return;
    }
    auto result = _prepareFn(static_cast<LuaApplicationModule*>(this));
    if(!result.valid()) {
      sol::error err = result;
      throw ChimeraTK::logic_error("Lua prepare error in '" + getName() + "': " + std::string(err.what()));
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::run() {
    // Script has already been loaded at construction time. Just start the module thread.
    ApplicationModule::run();
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
      // sol2 reports a non-std::exception (typically boost::thread_interrupted from a blocking
      // accessor read at shutdown) as the literal string "C++ exception". Treat that as a normal
      // termination signal — the only code path that throws non-std exceptions across the
      // sol2 boundary in this binding is the thread interrupt.
      if(msg.find("C++ exception") != std::string_view::npos) {
        return;
      }
      throw ChimeraTK::logic_error("Lua mainLoop error in '" + getName() + "': " + std::string(msg));
    }
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::terminate() {
    // ApplicationModule::terminate() joins the module thread, so by the time we
    // reach _moduleState.reset() no other thread can touch the Lua state.
    ApplicationModule::terminate();
    _moduleState.reset();
  }

  /********************************************************************************************************************/

  void LuaApplicationModule::bind(sol::state& lua) {
    lua.new_usertype<LuaApplicationModule>(
        "ApplicationModule", sol::no_constructor,
        // Fallback metamethods for dynamically-assigned fields. sol2 dispatches registered
        // members first; these only fire for keys that are not bound below — so
        // `mod:ScalarOutput(...)` still hits the C++ factory, while `mod.myOutput = X` and
        // `function mod:mainLoop()` land in the companion _attrs table.
        sol::meta_function::new_index,
        [](LuaApplicationModule& self, sol::this_state ts, sol::object key, sol::object value) {
          if(!self._attrs.valid()) {
            self._attrs = sol::state_view(ts).create_table();
          }
          self._attrs[key] = value;
        },
        sol::meta_function::index,
        [](LuaApplicationModule& self, sol::this_state ts, sol::object key) -> sol::object {
          if(self._attrs.valid()) {
            sol::object v = self._attrs[key];
            if(v.valid() && v.get_type() != sol::type::nil) {
              return v;
            }
          }
          return sol::make_object(ts, sol::lua_nil);
        },
        "getName", &LuaApplicationModule::getName, "readAll",
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
        "writeAllDestructively",
        [](LuaApplicationModule& self, sol::optional<bool> includeReturnChannels) {
          self.writeAllDestructively(includeReturnChannels.value_or(false));
        },
        "getCurrentVersionNumber", &LuaApplicationModule::getCurrentVersionNumber, "setCurrentVersionNumber",
        &LuaApplicationModule::setCurrentVersionNumber, "getDataValidity", &LuaApplicationModule::getDataValidity,
        "incrementDataFaultCounter", &LuaApplicationModule::incrementDataFaultCounter, "decrementDataFaultCounter",
        &LuaApplicationModule::decrementDataFaultCounter, "getDataFaultCounter",
        &LuaApplicationModule::getDataFaultCounter, "disable", &LuaApplicationModule::disable,

        // Scalar accessor factory methods (mod:ScalarPushInput(...))
        "ScalarPushInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPushInput>{}, type, &self, name, unit, description);
        },
        "ScalarPushInputWB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPushInputWB>{}, type, &self, name, unit, description);
        },
        "ScalarPollInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarPollInput>{}, type, &self, name, unit, description);
        },
        "ScalarOutput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutput>{}, type, &self, name, unit, description);
        },
        "ScalarOutputPushRB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutputPushRB>{}, type, &self, name, unit, description);
        },
        "ScalarOutputReverseRecovery",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            const std::string& description) -> LuaScalarAccessor& {
          return *self.make_child<LuaScalarAccessor>(
              AccessorTypeTag<ScalarOutputReverseRecovery>{}, type, &self, name, unit, description);
        },
        "ArrayPushInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPushInput>{}, type, &self, name, unit, nElements, description);
        },
        "ArrayPushInputWB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPushInputWB>{}, type, &self, name, unit, nElements, description);
        },
        "ArrayPollInput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayPollInput>{}, type, &self, name, unit, nElements, description);
        },
        "ArrayOutput",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutput>{}, type, &self, name, unit, nElements, description);
        },
        "ArrayOutputPushRB",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutputPushRB>{}, type, &self, name, unit, nElements, description);
        },
        "ArrayOutputReverseRecovery",
        [](LuaApplicationModule& self, ChimeraTK::DataType type, const std::string& name, const std::string& unit,
            size_t nElements, const std::string& description) -> LuaArrayAccessor& {
          return *self.make_child<LuaArrayAccessor>(
              AccessorTypeTag<ArrayOutputReverseRecovery>{}, type, &self, name, unit, nElements, description);
        },
        "VoidInput",
        [](LuaApplicationModule& self, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidInput>{}, &self, name, description);
        },
        "VoidOutput",
        [](LuaApplicationModule& self, const std::string& name, const std::string& description) -> LuaVoidAccessor& {
          return *self.make_child<LuaVoidAccessor>(VoidTypeTag<VoidOutput>{}, &self, name, description);
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
        "VariableGroup",
        [](LuaApplicationModule& self, const std::string& name,
            const std::string& description) -> LuaVariableGroup& {
          return *self.make_child<LuaVariableGroup>(&self, name, description);
        });

    // Factory function: ApplicationModule(name, description)
    // This factory function is registered in the module's lua state before script execution.
    // It returns a reference to the C++-created module instance that was injected as a global.
    // The parent group is implicit - it's already established on the C++ side.
    //
    // We capture lua_state() at registration time so we can access the global _modSelfRef later.
    lua_State* L = lua.lua_state();
    lua.set_function(
        "ApplicationModule", [L](const std::string& name, const std::string& description) -> LuaApplicationModule& {
          // Access the global _modSelfRef from the lua state where this was registered
          sol::state_view lua_view(L);
          sol::object modRef = lua_view.globals()["_modSelfRef"];

          if(modRef.get_type() == sol::type::nil) {
            throw ChimeraTK::logic_error("ApplicationModule(): module reference not injected - internal error");
          }

          if(!modRef.is<LuaApplicationModule>()) {
            throw ChimeraTK::logic_error("ApplicationModule(): module reference has wrong type - internal error");
          }

          return modRef.as<LuaApplicationModule&>();
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
