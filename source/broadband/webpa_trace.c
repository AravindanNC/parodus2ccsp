#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "webpa_trace.h"
#include "rdk_otlp_instrumentation.h"

static bool validTraceparent(const char *traceparent)
{
    size_t index;
    bool nonzeroTrace = false;
    bool nonzeroParent = false;

    if(traceparent == NULL || strlen(traceparent) != 55 ||
       traceparent[0] != '0' || traceparent[1] != '0' ||
       traceparent[2] != '-' || traceparent[35] != '-' || traceparent[52] != '-')
        return false;

    for(index = 3; index < 55; ++index)
    {
        char digit = traceparent[index];
        if(index == 35 || index == 52)
            continue;
        if(!((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f')))
            return false;
        if(index < 35 && digit != '0')
            nonzeroTrace = true;
        if(index > 35 && index < 52 && digit != '0')
            nonzeroParent = true;
    }
    return nonzeroTrace && nonzeroParent;
}

bool webpaTraceStart(const char *traceparent, const char *operation, const char *transactionId)
{
    if(!validTraceparent(traceparent) || operation == NULL ||
       access("/tmp/rdk_distributed_tracing_enabled", F_OK) != 0)
        return false;

    rdk_otlp_start_child_from_traceparent(traceparent, operation);
    if(rdk_otlp_get_current_traceparent() == NULL)
        return false;

    if(transactionId != NULL)
        rdk_otlp_set_span_attribute_string("webpa.transaction_id", transactionId);
    rdk_otlp_set_span_attribute_string("webpa.transport", "wrp");
    return true;
}

void webpaTraceFinish(bool started, const char *status)
{
    if(!started)
        return;
    if(status != NULL)
        rdk_otlp_set_span_attribute_string("webpa.status", status);
    rdk_otlp_finish_child_span();
}
