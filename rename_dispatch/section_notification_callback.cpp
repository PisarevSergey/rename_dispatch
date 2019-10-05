#include "common.h"

NTSTATUS section_notification_callback::callback(PFLT_INSTANCE /*Instance*/,
  PFLT_CONTEXT section_context,
  PFLT_CALLBACK_DATA data)
{
  section_context::context* ctx = static_cast<section_context::context*>(section_context);
  ctx->wait_for_finished_work(data);
  return STATUS_SUCCESS;
}
