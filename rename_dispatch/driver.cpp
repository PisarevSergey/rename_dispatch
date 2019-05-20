#include "common.h"
#include "driver.tmh"

namespace
{
  NTSTATUS unload(FLT_FILTER_UNLOAD_FLAGS)
  {
    info_message(DRIVER, "unloading");
    delete get_driver();
    return STATUS_SUCCESS;
  }

  class tracing_driver : public driver
  {
  public:
    tracing_driver()
    {
      WPP_INIT_TRACING(0, 0);
    }

    ~tracing_driver()
    {
      WPP_CLEANUP(0);
    }
  };

  class fltmgr_filter_driver : public tracing_driver
  {
  public:
    fltmgr_filter_driver(NTSTATUS& stat, PDRIVER_OBJECT drv)
    {
      FLT_REGISTRATION freg = { 0 };
      freg.Size = sizeof(freg);
      freg.Version = FLT_REGISTRATION_VERSION;
      freg.FilterUnloadCallback = unload;

      stat = FltRegisterFilter(drv, &freg, &filter);
    }

    ~fltmgr_filter_driver()
    {
      FltUnregisterFilter(filter);
    }

    NTSTATUS start_filtering() { return FltStartFiltering(filter); }
  private:
    PFLT_FILTER filter;
  };

  class top_driver : public fltmgr_filter_driver
  {
  public:
    top_driver(NTSTATUS& stat, PDRIVER_OBJECT drv) : fltmgr_filter_driver(stat, drv)
    {}

    void* __cdecl operator new(size_t, void* p) { return p; }
  };

  char driver_memory[sizeof(top_driver)];
}

driver* create_driver(NTSTATUS& stat, PDRIVER_OBJECT drv)
{
  stat = STATUS_UNSUCCESSFUL;
  auto d(new(driver_memory) top_driver(stat, drv));
  if (!NT_SUCCESS(stat))
  {
    delete d;
    d = 0;
  }

  return d;
}

driver* get_driver()
{
  return reinterpret_cast<driver*>(driver_memory);
}
