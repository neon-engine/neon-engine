#ifndef MOCK_SCRIPT_CONTEXT_HPP
#define MOCK_SCRIPT_CONTEXT_HPP

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/scripting/script-context.hpp>

namespace neon::testing
{
  class MockScriptContext : public ScriptContext
  {
  public:
    MOCK_METHOD(bool, LoadScripts, (const std::string &folder, EntityStore &store, ComponentFormats &formats), (override));

    MOCK_METHOD(void, Start, (EntityStore &store), (override));

    MOCK_METHOD(void, Update, (EntityStore &store, double delta_time), (override));

    MOCK_METHOD(void, FixedUpdate, (EntityStore &store, double fixed_delta_time), (override));

    MOCK_METHOD(void, DispatchPhysicsEvents, (EntityStore &store, const std::vector<PhysicsEvent> &events), (override));

    MOCK_METHOD(std::size_t, GetComponentCount, (), (const, override));

    MOCK_METHOD(std::size_t, GetSystemCount, (), (const, override));
  };
} // neon::testing

#endif //MOCK_SCRIPT_CONTEXT_HPP
