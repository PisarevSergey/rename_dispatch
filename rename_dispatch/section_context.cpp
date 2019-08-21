#include "common.h"
#include "section_context.tmh"

size_t section_context::get_size() { return sizeof(section_context::context); }

section_context::context* section_context::create_context(NTSTATUS& stat)
{
  PFLT_CONTEXT ctx(0);
  stat = get_driver()->allocate_context(FLT_SECTION_CONTEXT, section_context::get_size(), NonPagedPool, &ctx);
  if (NT_SUCCESS(stat))
  {
    info_message(SECTION_CONTEXT, "section context allocation success");
  }
  else
  {
    error_message(SECTION_CONTEXT, "failed to allocate section context with status %!STATUS!", stat);
    ctx = 0;
  }

  return static_cast<section_context::context*>(ctx);
}
