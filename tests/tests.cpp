#include "io/log.hpp"

// those should be cross-platform
#include <signal.h>
#include <setjmp.h>

static jmp_buf test_env;
static int last_signal;
static int succeeded = 0, failed = 0;
static void crash_handler(int sig) {
    longjmp(test_env, sig? sig : 1); // skip test and continue forth
}

#define RUN_TEST(func) { LOG_MSGF("Running test %s... ", #func); if ((last_signal = setjmp(test_env)) == 0) \
{ if (Test:: func()) { LOG_MSG("OK\n"); ++succeeded; } else ++failed; } else \
{ LOG_MSGF("Test %s crashed with signal %d!\n", #func, last_signal); ++failed; } }
#define TEST_ASSERT(cond) if (!(cond)) \
{ LOG_MSGF("Test %s aborted: assertion \"%s\" failed.\n", __func__, #cond); goto fail; }
#define TEST_FAIL(str) { LOG_MSGF("Test %s aborted: \"%s\".\n", __func__, #str); goto fail; }
#define TEST_EPILOGUE return true; fail: return false;

/* test includes */

#include "core/events.hpp"
#include "browser/context.hpp"

/* start tests */

namespace Test {

    bool Self() {
        TEST_ASSERT(true);
        TEST_EPILOGUE;
    }

    bool BrowserLifecycle() {
        ms::browser::Context ctx;
        ctx.init();
        ctx.free();
        TEST_EPILOGUE;
    }

    bool BrowserScriptProto() {
        ms::browser::Context ctx;
        ctx.init();

        auto* js = ctx.getScriptHost();
        js->patchRuntime();
        auto eval = js->evalScript(R"js(
            var rt = new Runtime();
            var screen = new Screen(rt);
            (screen instanceof Screen) && (screen.clear instanceof Function);
        )js");
        TEST_ASSERT(eval == "true");

        ctx.free();
        TEST_EPILOGUE;
    }

    bool BrowserDefaultObjects() {
        ms::browser::Context ctx;
        ctx.init();

        // make sure whatever JS engine we're running has the minimum set of complex types
        auto* js = ctx.getScriptHost();
        auto eval = js->evalScript(R"js(
            (Date !== undefined) && (Math !== undefined) && (BigInt !== undefined) && (Number !== undefined);
        )js");
        TEST_ASSERT(eval == "true");

        ctx.free();
        TEST_EPILOGUE;
    }

    bool EventConnection() {
        struct EventInvoker {
            ms::Event<> testEvent;
        } a;
        auto conn = a.testEvent.connect([&]() {}, ms::EventEnum::CONN_ONCE);
        TEST_ASSERT(a.testEvent.connections() == 1);
        
        a.testEvent.disconnect(conn.id);
        TEST_ASSERT(a.testEvent.connections() == 0);
        TEST_EPILOGUE;
    }

    bool AwaitEvent() {
        ms::EventQueue queue; // make local queue

        int intChange = 0;
        std::string strChange = "";

        struct EventInvoker {
            ms::Event<int, std::string const&> testEvent;
        } a;
        a.testEvent.connect([&](int i, std::string const& s) {
            intChange = i; strChange = s;
        }, ms::EventEnum::CONN_ONCE);
        a.testEvent.invokeDeferredQueue(1000, "Changed by lambda!", &queue);

        queue.flushNotifications();

        TEST_ASSERT(intChange == 1000);
        TEST_ASSERT(strChange == "Changed by lambda!");
        TEST_EPILOGUE;
    }

}

/* end tests */

int main(int argc, char* argv[]) {
    ms::io::LogDispatcher::get().addStandardOutput();
    signal(SIGILL, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    
    // list all tests
    RUN_TEST(Self);
    RUN_TEST(EventConnection);
    RUN_TEST(AwaitEvent);
    RUN_TEST(BrowserLifecycle);
    RUN_TEST(BrowserDefaultObjects);
    RUN_TEST(BrowserScriptProto);

    LOG_MSGF("\n%s Tests: %d succeded, %d failed.\n", failed == 0? "\u2705" : "\u274C", succeeded, failed);
    return failed > 0;
}
