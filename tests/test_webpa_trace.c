#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "webpa_trace.h"
#include "rdk_otlp_instrumentation.h"

static const char *activeTraceparent;
static const char *receivedParent;
static unsigned int starts;
static unsigned int finishes;
static bool tracerAvailable = true;
static bool tracingEnabled = true;
static const char *childTraceparent =
    "00-4bf92f3577b34da6a3ce929d0e0e4736-1234567890abcdef-01";

int access(const char *path, int mode)
{
    assert(strcmp(path, "/tmp/rdk_distributed_tracing_enabled") == 0);
    assert(mode == F_OK);
    return tracingEnabled ? 0 : -1;
}

void rdk_otlp_start_child_from_traceparent(const char *traceparent, const char *operation)
{
    assert(strcmp(operation, "webpa.set") == 0);
    receivedParent = traceparent;
    ++starts;
    activeTraceparent = tracerAvailable ? childTraceparent : NULL;
}

const char *rdk_otlp_get_current_traceparent(void)
{
    return activeTraceparent;
}

void rdk_otlp_set_span_attribute_string(const char *key, const char *value)
{
    assert(key != NULL && value != NULL);
    assert(activeTraceparent != NULL);
}

void rdk_otlp_finish_child_span(void)
{
    assert(activeTraceparent != NULL);
    ++finishes;
    activeTraceparent = NULL;
}

int main(void)
{
    const char *parent = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    const char *invalid[] = {
        NULL, "", "trace-id-only", "00-short",
        "00-00000000000000000000000000000000-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01",
        "00-4bf92f3577b34da6a3ce929d0e0e473g-00f067aa0ba902b7-01",
        "00-4BF92F3577B34DA6A3CE929D0E0E4736-00f067aa0ba902b7-01",
        "ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01",
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra"
    };
    size_t index;
    bool started = webpaTraceStart(parent, "webpa.set", "transaction-1");
    assert(started);
    assert(strcmp(receivedParent, parent) == 0);
    assert(strcmp(activeTraceparent, parent) != 0);
    assert(strncmp(activeTraceparent + 3, parent + 3, 32) == 0);
    webpaTraceFinish(started, "0");
    assert(finishes == 1 && activeTraceparent == NULL);

    for(index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
    {
        started = webpaTraceStart(invalid[index], "webpa.set", NULL);
        assert(!started);
        webpaTraceFinish(started, "failure");
        assert(activeTraceparent == NULL);
    }
    assert(starts == 1 && finishes == 1);

    started = webpaTraceStart("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00",
                             "webpa.set", NULL);
    assert(started);
    webpaTraceFinish(started, "failure");
    assert(starts == 2 && finishes == 2);

    tracerAvailable = false;
    started = webpaTraceStart(parent, "webpa.set", NULL);
    assert(!started);
    webpaTraceFinish(started, NULL);
    assert(finishes == 2 && activeTraceparent == NULL);
    tracerAvailable = true;
    tracingEnabled = false;
    started = webpaTraceStart(parent, "webpa.set", NULL);
    assert(!started && starts == 3);
    webpaTraceFinish(started, NULL);
    assert(finishes == 2);
    puts("WebPA trace validation and span lifecycle: PASS");
    return 0;
}
