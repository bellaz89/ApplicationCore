// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Logger.h"

#include <string>

namespace sol {
  class state;
} // namespace sol

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Small helper exposing logger(severity, context):log(message) to Lua. */
  class LuaLoggerStreamProxy {
   public:
    /** Construct a logging proxy for the given severity and context. */
    LuaLoggerStreamProxy(Logger::Severity severity, std::string context)
    : _severity(severity), _context(std::move(context)) {}

    /** Emit a message through the wrapped logger stream. */
    void log(const std::string& message) { ChimeraTK::logger(_severity, _context) << message; }

   private:
    Logger::Severity _severity;
    std::string _context;
  };

  /********************************************************************************************************************/

  /** Register free-function logging helpers in the Lua state. */
  class LuaLogger {
   public:
    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
