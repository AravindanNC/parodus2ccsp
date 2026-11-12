#ifndef WEBPA_TRACE_H
#define WEBPA_TRACE_H

#include <stdbool.h>

bool webpaTraceStart(const char *traceparent, const char *operation, const char *transactionId);
void webpaTraceFinish(bool started, const char *status);

#endif
