#include "common.h"
#include "referenced_reporter_process.tmh"

namespace
{
  class process_with_rundown_guard : public referenced_reporter_process::process
  {
  public:
    process_with_rundown_guard()
    {
      ExInitializeRundownProtection(&guard);
    }

    bool acquire() override
    {
      return (TRUE == ExAcquireRundownProtection(&guard));
    }

    void release() override
    {
      ExReleaseRundownProtection(&guard);
    }

  protected:
    void wait_for_release_and_stop_acquiring()
    {
      ExWaitForRundownProtectionRelease(&guard);
      ExRundownCompleted(&guard);
    }

  private:
    EX_RUNDOWN_REF guard;
  };

  class process_with_referenced_eprocess : public process_with_rundown_guard
  {
  public:
    process_with_referenced_eprocess(NTSTATUS& stat, HANDLE pid) : proc(0)
    {
      stat = PsLookupProcessByProcessId(pid, &proc);
      if (NT_SUCCESS(stat))
      {
        info_message(REFERENCED_REPORTER_PROCESS, "PsLookupProcessByProcessId success");
      }
      else
      {
        error_message(REFERENCED_REPORTER_PROCESS, "PsLookupProcessByProcessId failed with status %!STATUS!", stat);
        proc = nullptr;
      }
    }

    ~process_with_referenced_eprocess()
    {
      if (proc)
      {
        info_message(REFERENCED_REPORTER_PROCESS, "dereferencing reporter process");
        ObDereferenceObject(proc);
      }
    }

    PEPROCESS get_eproc() const override
    {
      return proc;
    }
  private:
    PEPROCESS proc;
  };

  class top_level_process : public process_with_referenced_eprocess
  {
  public:
    top_level_process(NTSTATUS& stat, HANDLE pid) : process_with_referenced_eprocess(stat, pid)
    {}

    ~top_level_process()
    {
      wait_for_release_and_stop_acquiring();
    }

    void* __cdecl operator new(size_t size)
    {
      return ExAllocatePoolWithTag(NonPagedPool, size, 'rper');
    }
  };
}

void __cdecl referenced_reporter_process::process::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}

referenced_reporter_process::process* referenced_reporter_process::create_process(NTSTATUS& stat, HANDLE pid)
{
  auto pr = new top_level_process(stat, pid);
  if (pr)
  {
    info_message(REFERENCED_REPORTER_PROCESS, "top_level_process allocated");
    if (NT_SUCCESS(stat))
    {
      info_message(REFERENCED_REPORTER_PROCESS, "top_level_process init success");
    }
    else
    {
      error_message(REFERENCED_REPORTER_PROCESS, "failed to init top_level_process with status %!STATUS!", stat);
      delete pr;
      pr = 0;
    }
  }
  else
  {
    error_message(REFERENCED_REPORTER_PROCESS, "failed to allocate top_level_process");
    stat = STATUS_INSUFFICIENT_RESOURCES;
  }

  return pr;
}
