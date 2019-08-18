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

  class driver_with_reporter_mgr : public tracing_driver
  {
  public:
    NTSTATUS set_new_reporter_proc(HANDLE pid)
    {
      return mgr.set_new_reporter_process(pid);
    }

    referenced_reporter_process::process* get_reporter_proc_ref()
    {
      return mgr.get_reporter_ref();
    }
  private:
    reporter_process_mgr::manager mgr;
  };

  class fltmgr_filter_driver : public driver_with_reporter_mgr
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

    PFLT_FILTER get_filter()
    {
      return filter;
    }

    NTSTATUS start_filtering() { return FltStartFiltering(filter); }

    NTSTATUS allocate_context(_In_ FLT_CONTEXT_TYPE ContextType, _In_ SIZE_T ContextSize, _In_ POOL_TYPE PoolType, _Out_ PFLT_CONTEXT *ReturnedContext)
    {
      return FltAllocateContext(filter, ContextType, ContextSize, PoolType, ReturnedContext);
    }
  protected:
    PFLT_FILTER filter;
  };

  class driver_with_communication_port : public fltmgr_filter_driver
  {
  public:
    driver_with_communication_port(NTSTATUS& stat, PDRIVER_OBJECT drv) : fltmgr_filter_driver(stat, drv), server_port(0)
    {
      do
      {
        if (!NT_SUCCESS(stat))
        {
          error_message(DRIVER, "fltmgr_filter_driver ctor returned failure status %!STATUS!", stat);
          break;
        }
        info_message(DRIVER, "fltmgr_filter_driver ctor success");

        PSECURITY_DESCRIPTOR sd(0);
        stat = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
        if (!NT_SUCCESS(stat))
        {
          error_message(DRIVER, "FltBuildDefaultSecurityDescriptor failed with status %!STATUS!", stat);
          break;
        }
        info_message(DRIVER, "FltBuildDefaultSecurityDescriptor success");

        UNICODE_STRING com_port_name = RTL_CONSTANT_STRING(um_km_communication::communication_port_name);

        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa,
                                   &com_port_name,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   0,
                                   sd);
        stat = FltCreateCommunicationPort(filter,
                                          &server_port,
                                          &oa,
                                          0,
                                          connect_client_port_notify,
                                          disconnect_client_port_notify,
                                          0,
                                          1);
        FltFreeSecurityDescriptor(sd);
        if (!NT_SUCCESS(stat))
        {
          error_message(DRIVER, "FltCreateCommunicationPort failed with status %!STATUS!", stat);
          server_port = 0;
          break;
        }
        info_message(DRIVER, "FltCreateCommunicationPort success");

      } while (false);

    }

    ~driver_with_communication_port()
    {
      verbose_message(DRIVER, "entering communication port dtor");
      if (server_port)
      {
        info_message(DRIVER, "closing communication port");
        FltCloseCommunicationPort(server_port);
      }
    }

    static NTSTATUS connect_client_port_notify(
        PFLT_PORT ClientPort,
        PVOID,
        PVOID,
        ULONG,
        PVOID*)
    {
      info_message(DRIVER, "client port connect");
      client_port = ClientPort;
      return STATUS_SUCCESS;
    }


    static void disconnect_client_port_notify(PVOID)
    {
      info_message(DRIVER, "closing client port");
      FltCloseClientPort(get_driver()->get_filter(), &client_port);
    }


  private:
    PFLT_PORT server_port;
  public:
    static PFLT_PORT client_port;
  };

  PFLT_PORT driver_with_communication_port::client_port = 0;

  class top_driver : public driver_with_communication_port
  {
  public:
    top_driver(NTSTATUS& stat, PDRIVER_OBJECT drv) : driver_with_communication_port(stat, drv)
    {}

    void* __cdecl operator new(size_t, void* p) { return p; }
  };

  char driver_memory[sizeof(top_driver)];
}

NTSTATUS send_message_to_um(void* send_buffer, ULONG send_buffer_length, PLARGE_INTEGER timeout)
{
  um_km_communication::reply_from_um dummy;
  ULONG reply_buffer_length(sizeof(dummy));
  return FltSendMessage(get_driver()->get_filter(),
                        &driver_with_communication_port::client_port,
                        send_buffer,
                        send_buffer_length,
                        &dummy,
                        &reply_buffer_length,
                        timeout);
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
