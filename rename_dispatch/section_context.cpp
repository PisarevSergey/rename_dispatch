#include "common.h"
#include "section_context.tmh"

void section_context::cleanup(PFLT_CONTEXT /*ctx*/, FLT_CONTEXT_TYPE /*ctx_type*/)
{}

constexpr size_t section_context::get_size() { return sizeof(section_context::context); }

section_context::context* section_context::create_context(NTSTATUS& stat)
{
  PFLT_CONTEXT ctx(0);
  stat = get_driver()->allocate_context(FLT_SECTION_CONTEXT, section_context::get_size(), NonPagedPool, &ctx);
  if (NT_SUCCESS(stat))
  {
    info_message(SECTION_CONTEXT, "section context allocation success");
    //RtlZeroMemory(ctx, section_context::get_size());
    KeInitializeEvent(&static_cast<section_context::context*>(ctx)->work_finished, NotificationEvent, FALSE);
  }
  else
  {
    error_message(SECTION_CONTEXT, "failed to allocate section context with status %!STATUS!", stat);
    ctx = 0;
  }

  return static_cast<section_context::context*>(ctx);
}

void section_context::context::wait_for_finished_work(PFLT_CALLBACK_DATA data)
{
  LARGE_INTEGER timeout = support::seconds_to_100_nanosec_units(-report_time_to_live_in_seconds);
  FltCancellableWaitForSingleObject(&work_finished, &timeout, data);
}
