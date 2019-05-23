#pragma once

class driver
{
public:
  virtual NTSTATUS start_filtering() = 0;
  virtual NTSTATUS allocate_context(_In_ FLT_CONTEXT_TYPE ContextType, _In_ SIZE_T ContextSize, _In_ POOL_TYPE PoolType, _Out_ PFLT_CONTEXT *ReturnedContext) = 0;

  virtual ~driver() {}
  void __cdecl operator delete(void*) {}
};

driver* create_driver(NTSTATUS& stat, PDRIVER_OBJECT);
driver* get_driver();