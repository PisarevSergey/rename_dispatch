#pragma once

namespace referenced_reporter_process
{
  class process
  {
  public:
    virtual ~process() {}

    void __cdecl operator delete(void* p);

    virtual bool acquire() = 0;
    virtual void release() = 0;

    virtual PEPROCESS get_eproc() const = 0;
  };

  referenced_reporter_process::process* create_process(NTSTATUS& stat, HANDLE pid);
}
