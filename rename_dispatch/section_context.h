#pragma once

namespace section_context
{
  struct context
  {
    void wait_for_finished_work(PFLT_CALLBACK_DATA data);

    KEVENT work_finished;
  };

  void cleanup(PFLT_CONTEXT ctx, FLT_CONTEXT_TYPE ctx_type);
  constexpr size_t get_size();

  context* create_context(NTSTATUS& stat);
}
