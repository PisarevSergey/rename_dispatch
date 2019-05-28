#pragma once

namespace stream_context
{
  class context
  {
  public:
    virtual ~context() {}

    virtual void insert_rename_info(rename_info::info* ren_info) = 0;
    virtual rename_info::info* extract_rename_info_by_thread(PETHREAD thread) = 0;

    void* __cdecl operator new(size_t, void* p) { return p; }
    void __cdecl operator delete(void*) {}
  };

  void cleanup(PFLT_CONTEXT Context, FLT_CONTEXT_TYPE ContextType);

  size_t get_size();

  context* create_context(NTSTATUS& stat);
}
