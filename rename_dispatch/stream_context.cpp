#include "common.h"
#include "stream_context.tmh"

namespace
{
  class top_stream_context : public stream_context::context
  {
  public:
    top_stream_context(NTSTATUS& stat)
    {
      stat = STATUS_SUCCESS;
    }
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
    new (ctx) top_stream_context(stat);
    if (NT_SUCCESS(stat))
    {
      info_message(STREAM_CONTEXT, "stream context initialization success");
    }
    else
    {
      FltReleaseContext(ctx);
      ctx = 0;
      error_message(STREAM_CONTEXT, "failed to initialize stream context with status %!STATUS!", stat);
    }
  }
  else
  {
    error_message(STREAM_CONTEXT, "failed to allocate stream context with status %!STATUS!", stat);
    ctx = 0;
  }

  return static_cast<stream_context::context*>(ctx);
}
