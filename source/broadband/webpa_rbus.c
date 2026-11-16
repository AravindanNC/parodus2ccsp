#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"
#ifdef TP_ENABLED
#include <strings.h>
#endif

static rbusHandle_t rbus_handle;
static bool isRbus = false;

bool isRbusEnabled()
{
        if(RBUS_ENABLED == rbus_checkStatus())
        {
                isRbus = true;
        }
        else
        {
                isRbus = false;
        }
        WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
        return isRbus;
}

bool isRbusInitialized()
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName)
{
        int ret = RBUS_ERROR_SUCCESS;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return WDMP_FAILURE;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return WDMP_SUCCESS;
}

void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
    rbus_handle = NULL;
}

rbusError_t setTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        /* CID-334852 Function address comparison fix */
        if(isRbusInitialized())
        {
                  if(traceContext != NULL && traceContext[0] != NULL) {
                          if(strlen(traceContext[0]) > 0) {
                                                                const char *traceParent = traceContext[0];
                                const char *traceState = traceContext[1] != NULL ? traceContext[1] : "";
#ifdef TP_ENABLED
                                                                traceParent += strspn(traceParent, " \t");
                                                                if(strncasecmp(traceParent, "traceparent:", sizeof("traceparent:") - 1) == 0)
                                                                        traceParent += sizeof("traceparent:") - 1;
                                                                traceParent += strspn(traceParent, " \t");
                                                                traceState += strspn(traceState, " \t");
                                                                if(strncasecmp(traceState, "tracestate:", sizeof("tracestate:") - 1) == 0)
                                                                        traceState += sizeof("tracestate:") - 1;
                                                                traceState += strspn(traceState, " \t");
#endif
			    WalInfo("Invoked setTraceContext function with value traceParent - %s, traceState - %s\n", traceParent, traceState);
                                                                ret = rbusHandle_SetTraceContextFromString(rbus_handle, traceParent, traceState);
                            if(ret == RBUS_ERROR_SUCCESS) {
                                  WalPrint("SetTraceContext request success\n");
                            }
                             else {
                                   WalError("SetTraceContext request failed with error code - %d\n", ret);
                             }
                        }
                        else {
                              WalError("Header is empty\n");
                        }
                  }
                  else {
                        WalError("Header is NULL\n");
                  }
        }
        else {
                WalError("Rbus not initialzed in setTraceContext function\n");
        }	
        return ret;
}

rbusError_t getTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        char traceParent[512] = {'\0'};
        char traceState[512] = {'\0'};
	if(traceContext == NULL)
                return RBUS_ERROR_INVALID_INPUT;
	/* CID-334847 Function address comparison fix */
	if(isRbusInitialized())
        {
	      ret =  rbusHandle_GetTraceContextAsString(rbus_handle, traceParent, sizeof(traceParent), traceState, sizeof(traceState));
	      if( ret == RBUS_ERROR_SUCCESS) {
		      if(strlen(traceParent) > 0) {
			      WalPrint("GetTraceContext request success\n");
#ifdef TP_ENABLED
                              char parentHeader[sizeof(traceParent) + sizeof("traceparent: ")];
                              char stateHeader[sizeof(traceState) + sizeof("tracestate: ")];
                              snprintf(parentHeader, sizeof(parentHeader), "%s%s",
                                       strncasecmp(traceParent, "traceparent:", sizeof("traceparent:") - 1) == 0 ? "" : "traceparent: ", traceParent);
                              snprintf(stateHeader, sizeof(stateHeader), "%s%s",
                                       strncasecmp(traceState, "tracestate:", sizeof("tracestate:") - 1) == 0 ? "" : "tracestate: ", traceState);
                              traceContext[0] = strdup(parentHeader);
                              traceContext[1] = strdup(stateHeader);
#else
		              traceContext[0] = strdup(traceParent);
	                      traceContext[1] = strdup(traceState);
#endif
			      if(traceContext[0] == NULL || traceContext[1] == NULL) {
                                      free(traceContext[0]);
                                      free(traceContext[1]);
                                      traceContext[0] = NULL;
                                      traceContext[1] = NULL;
                                      return RBUS_ERROR_OUT_OF_RESOURCES;
                              }
			      WalInfo("traceContext value, traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);
	               }
		       else {
                               WalPrint("traceParent is empty\n");
		       }	       
	      }
	      else {
		      WalError("GetTraceContext request failed with error code - %d\n", ret);
	      }	      
	}
        else { 
              WalError("Rbus not initialzed in getTraceContext function\n");
	}
        return ret;
}

rbusError_t clearTraceContext()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	/* CID-334849 Function address comparison fix */
	if(isRbusInitialized())
	{
		ret = rbusHandle_ClearTraceContext(rbus_handle);
		if(ret == RBUS_ERROR_SUCCESS) {
			WalInfo("ClearTraceContext request success\n");
		}
		else {
			WalError("ClearTraceContext request failed with error code - %d\n", ret);
		}
	}
	else {
		WalError("Rbus not initialized in clearTraceContext funcion\n");
        }
        /* CID-334846 missing return statement fix */
        return ret;
}

rbusError_t webpaRbusMethodInvoke(const char *methodName, rbusObject_t inParams, rbusObject_t *outParams)
{
        if(!isRbusInitialized())
        {
                WalError("Rbus not initialized in webpaRbusMethodInvoke function\n");
                return RBUS_ERROR_NOT_INITIALIZED;
        }
        if(methodName == NULL || outParams == NULL)
        {
                WalError("Invalid arguments to webpaRbusMethodInvoke\n");
                return RBUS_ERROR_INVALID_INPUT;
        }
        WalInfo("RBUS method invoke to %s synchronously\n", methodName);
        return rbusMethod_Invoke(rbus_handle, methodName, inParams, outParams);
}

