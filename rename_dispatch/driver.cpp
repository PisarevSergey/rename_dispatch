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
    attach(_In_ PCFLT_RELATED_OBJECTS objs,
           _In_ FLT_INSTANCE_SETUP_FLAGS,
           _In_ DEVICE_TYPE VolumeDeviceType,
           _In_ FLT_FILESYSTEM_TYPE)
  {
    NTSTATUS stat(STATUS_FLT_DO_NOT_ATTACH);

    if ((FILE_DEVICE_DISK_FILE_SYSTEM    == VolumeDeviceType) ||
        (FILE_DEVICE_NETWORK_FILE_SYSTEM == VolumeDeviceType))
    {
      const NTSTATUS register_for_data_scan_stat(FltRegisterForDataScan(objs->Instance));
      if (NT_SUCCESS(register_for_data_scan_stat))
      {
        stat = STATUS_SUCCESS;
        info_message(DRIVER, "FltRegisterForDataScan success");
      }
      else
      {
        error_message(DRIVER, "FltRegisterForDataScan failed with status %!STATUS!", register_for_data_scan_stat);
      }
    }

    return stat;
  }

  NTSTATUS section_notification_callback(PFLT_INSTANCE /*Instance*/,
    PFLT_CONTEXT section_context,
    PFLT_CALLBACK_DATA data)
  {
    section_context::context* ctx = static_cast<section_context::context*>(section_context);
    ctx->wait_for_finished_work(data);
    return STATUS_SUCCESS;
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

    void reset_reporter_proc()
    {
      mgr.reset_reporter_process();
    }

    referenced_reporter_process::process* get_reporter_proc_ref()
    {
      return mgr.get_reporter_ref();
    }
  private:
    reporter_process_mgr::manager mgr;
  };

  class driver_with_reporter : public driver_with_reporter_mgr
  {
  public:
    driver_with_reporter(NTSTATUS& stat) : r(nullptr)
    {
      r = reporter_facility::create_reporter(stat);
      if (NT_SUCCESS(stat))
      {
        info_message(DRIVER, "reporter_facility::create_reporter success");
      }
      else
      {
        error_message(DRIVER, "reporter_facility::create_reporter failed with status %!STATUS!", stat);
      }
    }

    ~driver_with_reporter()
    {
      delete r;
    }

    reporter_facility::reporter* get_reporter()
    {
      return r;
    }
  private:
    reporter_facility::reporter* r;
  };

  class fltmgr_filter_driver : public driver_with_reporter
  {
  public:
    fltmgr_filter_driver(NTSTATUS& stat, PDRIVER_OBJECT drv) : driver_with_reporter(stat), filter(0)
    {
      FLT_REGISTRATION freg = { 0 };
      freg.Size = sizeof(freg);
      freg.Version = FLT_REGISTRATION_VERSION;
      freg.FilterUnloadCallback = unload;
      freg.InstanceSetupCallback = attach;
      freg.SectionNotificationCallback = section_notification_callback;

      const FLT_CONTEXT_REGISTRATION context_reg[] =
      {
        {FLT_STREAM_CONTEXT , 0, stream_context::cleanup , stream_context::get_size() , 'crtS'},
        {FLT_SECTION_CONTEXT, 0, section_context::cleanup, section_context::get_size(), 'tceS'},
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
                                          message_notify,
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
        PVOID connection_ctx,
        ULONG connection_ctx_size,
        PVOID*)
    {
      info_message(DRIVER, "client port connect");

      NTSTATUS stat;

      um_km_communication::connection_context* cc(static_cast<um_km_communication::connection_context*>(connection_ctx));
      if (connection_ctx_size >= sizeof(*cc))
      {
        info_message(DRIVER, "connection context size ok");
        stat = get_driver()->set_new_reporter_proc(reinterpret_cast<HANDLE>(cc->reporter_pid));
        if (NT_SUCCESS(stat))
        {
          info_message(DRIVER, "set_new_reporter_proc success");
          client_port = ClientPort;
        }
        else
        {
          error_message(DRIVER, "set_new_reporter_proc failed with status %!STATUS!", stat);
        }
      }
      else
      {
        error_message(DRIVER, "connection context size too small");
        stat = STATUS_INVALID_PARAMETER;
      }

      return stat;
    }


    static void disconnect_client_port_notify(PVOID)
    {
      info_message(DRIVER, "closing client port");
      FltCloseClientPort(get_driver()->get_filter(), &client_port);
      get_driver()->reset_reporter_proc();
    }

    static NTSTATUS message_notify(PVOID /*PortCookie*/,
                                   PVOID InputBuffer OPTIONAL,
                                   ULONG InputBufferLength,
                                   PVOID /*OutputBuffer */,
                                   ULONG /*OutputBufferLength*/,
                                   PULONG ReturnOutputBufferLength)
    {
      NTSTATUS stat(STATUS_UNSUCCESSFUL);

      *ReturnOutputBufferLength = 0;

      um_km_communication::release_report_message release_msg = { 0 };
      __try
      {
        if (InputBufferLength >= sizeof(release_msg))
        {
          RtlCopyMemory(&release_msg, InputBuffer, sizeof(release_msg));
          stat = STATUS_SUCCESS;
        }
      }
      __except (EXCEPTION_EXECUTE_HANDLER)
      {
        stat = GetExceptionCode();
      }

      if (NT_SUCCESS(stat))
      {
        info_message(DRIVER, "user mode parameters validation success");
        delete get_driver()->get_reporter()->get_list_waiting_for_cleaning()->pop_report_by_number(release_msg.report_number_to_release);
      }
      else
      {
        error_message(DRIVER, "user mode parameters validation failed with status %!STATUS!", stat);
      }

      return stat;
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
  return FltSendMessage(get_driver()->get_filter(),
                        &driver_with_communication_port::client_port,
                        send_buffer,
                        send_buffer_length,
                        0,
                        0,
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
