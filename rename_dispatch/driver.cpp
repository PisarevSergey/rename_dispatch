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

  NTSTATUS
    attach(_In_ PCFLT_RELATED_OBJECTS,
           _In_ FLT_INSTANCE_SETUP_FLAGS,
           _In_ DEVICE_TYPE VolumeDeviceType,
           _In_ FLT_FILESYSTEM_TYPE)
  {
    NTSTATUS stat(STATUS_FLT_DO_NOT_ATTACH);

    if ((FILE_DEVICE_DISK_FILE_SYSTEM    == VolumeDeviceType) ||
        (FILE_DEVICE_NETWORK_FILE_SYSTEM == VolumeDeviceType))
    {
      stat = STATUS_SUCCESS;
    }

    return stat;
  }

  class tracing_driver : public driver
  {
  public:
    tracing_driver()
    {
      WPP_INIT_TRACING(0, 0);
      verbose_message(DRIVER, "tracing started");
    }

    ~tracing_driver()
    {
      verbose_message(DRIVER, "stopping tracing");
      WPP_CLEANUP(0);
    }
  };

  class fltmgr_filter_driver : public tracing_driver
  {
  public:
    fltmgr_filter_driver(NTSTATUS& stat, PDRIVER_OBJECT drv) : filter(0)
    {
      FLT_REGISTRATION freg = { 0 };
      freg.Size = sizeof(freg);
      freg.Version = FLT_REGISTRATION_VERSION;
      freg.FilterUnloadCallback = unload;
      freg.InstanceSetupCallback = attach;

      FLT_CONTEXT_REGISTRATION context_reg[] =
      {
        {FLT_STREAM_CONTEXT, 0, stream_context::cleanup, stream_context::get_size(), 'crtS'},
        {FLT_CONTEXT_END}
      };

      freg.ContextRegistration = context_reg;
      freg.OperationRegistration = operations::get_registration();

      stat = FltRegisterFilter(drv, &freg, &filter);
      if (NT_SUCCESS(stat))
      {
        info_message(DRIVER, "FltRegisterFilter success");
      }
      else
      {
        filter = 0;
        error_message(DRIVER, "FltRegisterFilter failed with status %!STATUS!", stat);
      }
    }

    ~fltmgr_filter_driver()
    {
      if (filter)
      {
        info_message(DRIVER, "unregistering filter");
        FltUnregisterFilter(filter);
      }
      else
      {
        info_message(DRIVER, "filter wasn't registered");
      }
    }

    NTSTATUS start_filtering() { return FltStartFiltering(filter); }

    NTSTATUS allocate_context(_In_ FLT_CONTEXT_TYPE ContextType, _In_ SIZE_T ContextSize, _In_ POOL_TYPE PoolType, _Out_ PFLT_CONTEXT *ReturnedContext)
    {
      return FltAllocateContext(filter, ContextType, ContextSize, PoolType, ReturnedContext);
    }
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
  if (NT_SUCCESS(stat))
  {
    info_message(DRIVER, "driver ctor success");
  }
  else
  {
    error_message(DRIVER, "driver ctor failed with status %!STATUS!", stat);
    delete d;
    d = 0;
  }

  return d;
}

driver* get_driver()
{
  return reinterpret_cast<driver*>(driver_memory);
}
