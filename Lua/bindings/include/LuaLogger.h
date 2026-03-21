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

  class LuaLoggerStreamProxy {
   public:
    LuaLoggerStreamProxy(Logger::Severity severity, std::string context)
    : _severity(severity), _context(std::move(context)) {}

    void log(const std::string& message) { ChimeraTK::logger(_severity, _context) << message; }

   private:
    Logger::Severity _severity;
    std::string _context;
  };

  /********************************************************************************************************************/

  class LuaLogger {
   public:
    static void bind(sol::state& lua);
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
