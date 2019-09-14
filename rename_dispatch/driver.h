#pragma once

class driver
{
public:
  virtual NTSTATUS start_filtering() = 0;
  virtual NTSTATUS allocate_context(_In_ FLT_CONTEXT_TYPE ContextType, _In_ SIZE_T ContextSize, _In_ POOL_TYPE PoolType, _Out_ PFLT_CONTEXT *ReturnedContext) = 0;
  virtual PFLT_FILTER get_filter() = 0;
  virtual NTSTATUS set_new_reporter_proc(HANDLE pid) = 0;
  virtual void reset_reporter_proc() = 0;
  virtual referenced_reporter_process::process* get_reporter_proc_ref() = 0;
  virtual reporter_facility::reporter* get_reporter() = 0;

  virtual ~driver() {}
  void __cdecl operator delete(void*) {}
};

driver* create_driver(NTSTATUS& stat, PDRIVER_OBJECT);
driver* get_driver();

NTSTATUS send_message_to_um(void* send_buffer, ULONG send_buffer_length, PLARGE_INTEGER timeout);