// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <list>
#include <memory>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** Base class for all Lua-exposed objects which can be owned by another Lua/C++ object. */
  class LuaOwnedObject {
   public:
    virtual ~LuaOwnedObject() = default;
  };

  /********************************************************************************************************************/

  /**
   * Base class for Lua-exposed objects which can own other objects and can in
   * turn be owned by exactly one parent object.
   */
  class LuaOwningObject : public LuaOwnedObject {
   public:
    /**
     * Create an owned child object and return a non-owning pointer to it.
     *
     * Ownership remains with this LuaOwningObject instance until destruction.
     */
    template<class Child, typename... Args>
    Child* make_child(Args... args) { // NOLINT(readability-identifier-naming)
      auto ptr = std::make_unique<Child>(args...);
      Child* rv = ptr.get();

      _children.emplace_back(std::move(ptr));
      return rv;
    }

   private:
    // Note about ownership and deinit problem.
    // When naively mapping VariableGroup to Lua, we run into a problem at the deinitialization phase:
    // The accessors held by the VariableGroup reference back to their owner (the VariableGroup) and
    // in their C++ destructor, they actually call functions of the owner.
    // On the other hand, if a container in Lua is destroyed, Lua first releases the container
    // and then the elements. This is different from a C++ class holding the elements as class members.
    // To solve the issue, we decided to take away ownership handling from Lua and explicitly take
    // care on the C++ side. This requires some ownership lists (below) and handing out accessors
    // in sol2 as references (non-owning).
    std::list<std::unique_ptr<LuaOwnedObject>> _children;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
