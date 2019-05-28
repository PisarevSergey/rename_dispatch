#include "common.h"
#include "stream_context.tmh"

namespace
{
  class rename_infos_list : public support::list<rename_info::info>
  {
  public:
    void insert_rename_info(rename_info::info* ren_info)
    {
      lock();

      simple_push_unsafe(ren_info);

      unlock();
    }

    rename_info::info* extract_rename_info_by_thread(PETHREAD t)
    {
      rename_info::info* ren_info(0);

      lock();

      for (rename_info::info* i = static_cast<rename_info::info*>(head.Flink); i != &head; i = static_cast<rename_info::info*>(i->Flink))
      {
        if (i->get_thread() == t)
        {
          RemoveEntryList(i);
          ren_info = i;
          break;
        }
      }

      unlock();

      return ren_info;
    }
  };

  class stream_context_with_rename_infos : public stream_context::context
  {
  public:
    void insert_rename_info(rename_info::info* ren_info)
    {
      ren_info_list.insert_rename_info(ren_info);
    }

    rename_info::info* extract_rename_info_by_thread(PETHREAD thread)
    {
      return ren_info_list.extract_rename_info_by_thread(thread);
    }
  private:
    rename_infos_list ren_info_list;
  };

  class top_stream_context : public stream_context_with_rename_infos
  {
  public:
    top_stream_context(NTSTATUS& stat)
    {
      stat = STATUS_SUCCESS;
    }
  };
}

size_t stream_context::get_size() { return sizeof(top_stream_context); }

void stream_context::cleanup(PFLT_CONTEXT Context, FLT_CONTEXT_TYPE)
{
  delete static_cast<stream_context::context*>(Context);
}

stream_context::context* stream_context::create_context(NTSTATUS& stat)
{
  PFLT_CONTEXT ctx(0);
  stat = get_driver()->allocate_context(FLT_STREAM_CONTEXT, stream_context::get_size(), NonPagedPool, &ctx);
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
