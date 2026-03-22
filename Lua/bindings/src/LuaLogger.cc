// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LuaLogger.h"

#include <sol/sol.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void LuaLogger::bind(sol::state& lua) {
    lua.new_enum("Severity",
        "trace", Logger::Severity::trace,
        "debug", Logger::Severity::debug,
        "info", Logger::Severity::info,
        "warning", Logger::Severity::warning,
        "error", Logger::Severity::error);

    lua.new_usertype<LuaLoggerStreamProxy>("LoggerStream",
        sol::no_constructor,
        "log", &LuaLoggerStreamProxy::log);

    lua.set_function("logger",
        [](Logger::Severity severity, const std::string& context) {
          return LuaLoggerStreamProxy(severity, context);
        });

    // New API: log(severity, context, message) free function
    lua.set_function("log",
        [](Logger::Severity severity, const std::string& context, const std::string& message) {
          ChimeraTK::logger(severity, context) << message;
        });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
