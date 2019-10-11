#include "common.h"

FLT_PREOP_CALLBACK_STATUS cleanup_dispatch::pre(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS /*FltObjects*/, PVOID* /*CompletionContext*/)
{
  delay_operation::do_delay(data);

  return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
