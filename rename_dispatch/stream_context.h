#pragma once

namespace stream_context
{
  class context
  {
  public:
    virtual ~context() {}

    void* __cdecl operator new(size_t, void* p) { return p; }
    void __cdecl operator delete(void*) {}
  };

  void cleanup(PFLT_CONTEXT Context, FLT_CONTEXT_TYPE ContextType);

  context* create_context(NTSTATUS& stat);
}
