#include "common.h"
#include "stream_context.tmh"

namespace
{
  class top_stream_context : public stream_context::context
  {
  };
}

void stream_context::cleanup(PFLT_CONTEXT Context, FLT_CONTEXT_TYPE)
{
  delete static_cast<stream_context::context*>(Context);
}

stream_context::context* stream_context::create_context(NTSTATUS& stat)
{
  PFLT_CONTEXT ctx(0);
  stat = get_driver()->allocate_context(FLT_STREAM_CONTEXT, sizeof(top_stream_context), NonPagedPool, &ctx);
  if (NT_SUCCESS(stat))
  {
    info_message(STREAM_CONTEXT, "stream context allocation success");
    new (ctx) top_stream_context;
  }
  else
  {
    error_message(STREAM_CONTEXT, "failed to allocate stream context with status %!STATUS!", stat);
    ctx = 0;
  }

  return static_cast<stream_context::context*>(ctx);
}
