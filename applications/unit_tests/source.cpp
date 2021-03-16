#define LEGION_ENTRY
#define LEGION_SHOW_CONSOLE

#include <core/core.hpp>

#define DOCTEST_CONFIG_IMPLEMENT

#include "doctest.h"
#include "tests/core/filesystem.hpp"
#include "tests/core/ecs.hpp"

using namespace legion;

class Exitus : public Module {
public:
    void setup() override
    {
        reportSystem<ExitHelper>();
    }

    priority_type priority() override { return PRIORITY_MAX; };

    class ExitHelper : public System<ExitHelper>
    {
    public:
        void setup() override
        {
            //raiseEvent<events::exit>();
        }
    };
};

void LEGION_CCONV reportModules(Engine* engine)
{
    doctest::Context ctx;
    ctx.applyCommandLine(engine->getCliArgs().size(), engine->getCliArgs().data());

    engine->exitCode = ctx.run();

    if (ctx.shouldExit())
        engine->reportModule<Exitus>();
    //std::exit(res);

// additional application code
}
