#include "common.h"
#include "um_report_class.tmh"

extern "C" POBJECT_TYPE* MmSectionObjectType;

namespace
{
  class report_with_reporter_process : public um_report_class::report
  {
  public:
    report_with_reporter_process(NTSTATUS& stat) : reporter(get_driver()->get_reporter_proc_ref())
    {
      if (reporter)
      {
        info_message(UM_REPORT_CLASS, "reporter process active");
        stat = STATUS_SUCCESS;
      }
      else
      {
        error_message(UM_REPORT_CLASS, "process-reporter absent");
        stat = STATUS_UNSUCCESSFUL;
      }
    }

    ~report_with_reporter_process()
    {
      if (reporter)
      {
        reporter->release();
      }
    }
  protected:
    referenced_reporter_process::process* reporter;
  };

  class report_with_section_context : public report_with_reporter_process
  {
  public:
    report_with_section_context(NTSTATUS& stat) : report_with_reporter_process(stat), sec_ctx(nullptr)
    {
      if (NT_SUCCESS(stat))
      {
        sec_ctx = section_context::create_context(stat);
        if (NT_SUCCESS(stat))
        {
          info_message(UM_REPORT_CLASS, "section_context::create_context success");
        }
        else
        {
          ASSERT(0 == sec_ctx);
          error_message(UM_REPORT_CLASS, "section_context::create_context failed with status %!STATUS!", stat);
        }
      }
    }

    ~report_with_section_context()
    {
      if (sec_ctx)
      {
        KeSetEvent(&sec_ctx->work_finished, IO_NO_INCREMENT, FALSE);
        FltReleaseContext(sec_ctx);
        sec_ctx = 0;
      }
    }
  protected:
    section_context::context* sec_ctx;
  };

  class report_with_section : public report_with_section_context
  {
  public:
    report_with_section(NTSTATUS& stat, PFLT_CALLBACK_DATA data) : report_with_section_context(stat),
                                                                   section_context_was_attached(false),
                                                                   kernel_section_handle(0),
                                                                   kernel_section_object(nullptr)
    {
      do
      {
        if (NT_SUCCESS(stat))
        {
          info_message(UM_REPORT_CLASS, "report_with_section_context success, continue initialization");
        }
        else
        {
          error_message(UM_REPORT_CLASS, "report_with_section_context failed with status %!STATUS!, exiting", stat);
          break;
        }

        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa, 0, OBJ_KERNEL_HANDLE, 0, 0);

        stat = FltCreateSectionForDataScan(data->Iopb->TargetInstance,
          data->Iopb->TargetFileObject,
          sec_ctx,
          SECTION_MAP_READ,
          &oa,
          0,
          PAGE_READONLY,
          SEC_COMMIT,
          0,
          &kernel_section_handle,
          &kernel_section_object,
          &size_of_mapped_file);
        if (NT_SUCCESS(stat))
        {
          info_message(UM_REPORT_CLASS, "FltCreateSectionForDataScan success");
          section_context_was_attached = true;
        }
        else
        {
          error_message(UM_REPORT_CLASS, "FltCreateSectionForDataScan failed with status %!STATUS!", stat);
          kernel_section_handle = 0;
          kernel_section_object = 0;
          break;
        }
      } while (false);
    }

    ~report_with_section()
    {
      if (section_context_was_attached)
      {
        FltCloseSectionForDataScan(sec_ctx);
      }

      if (kernel_section_handle)
      {
        ZwClose(kernel_section_handle);
      }

      if (kernel_section_object)
      {
        ObDereferenceObject(kernel_section_object);
      }
    }
  private:
    HANDLE kernel_section_handle;
  protected:
    void* kernel_section_object;
  private:
    bool section_context_was_attached;

  protected:
    LARGE_INTEGER size_of_mapped_file;
  };

  class report_with_um_report : public report_with_section
  {
  public:
    report_with_um_report(NTSTATUS& stat, PFLT_CALLBACK_DATA data) : report_with_section(stat, data)
    {
      RtlZeroMemory(&um_rename_report, sizeof(um_rename_report));

      if (NT_SUCCESS(stat))
      {
        static LONG64 report_number(0);
        um_rename_report.report_number = InterlockedIncrement64(&report_number);

        um_rename_report.size_of_mapped_file = size_of_mapped_file;

        um_rename_report.pid = PsGetProcessId(IoThreadToProcess(data->Thread));
        info_message(UM_REPORT_CLASS, "pid: %p", um_rename_report.pid);

        um_rename_report.tid = PsGetThreadId(data->Thread);
        info_message(UM_REPORT_CLASS, "tid: %p", um_rename_report.tid);

        KeQuerySystemTime(&um_rename_report.time);

        BOOLEAN copy_on_open;
        BOOLEAN effective_only;
        SECURITY_IMPERSONATION_LEVEL imp_lvl;
        PACCESS_TOKEN imp_token = PsReferenceImpersonationToken(data->Thread, &copy_on_open, &effective_only, &imp_lvl);
        if (imp_token)
        {
          info_message(UM_REPORT_CLASS, "thread impersonating");
          stat = SeQueryAuthenticationIdToken(imp_token, &um_rename_report.auth_id);
          if (NT_SUCCESS(stat))
          {
            info_message(UM_REPORT_CLASS, "SeQueryAuthenticationIdToken success");
          }
          else
          {
            error_message(UM_REPORT_CLASS, "SeQueryAuthenticationIdToken failed with status %!STATUS!", stat);
          }

          PsDereferenceImpersonationToken(imp_token);
        }
        else
        {
          info_message(UM_REPORT_CLASS, "thread is not impersonating");
          PACCESS_TOKEN prim_token = PsReferencePrimaryToken(IoThreadToProcess(data->Thread));
          stat = SeQueryAuthenticationIdToken(prim_token, &um_rename_report.auth_id);
          if (NT_SUCCESS(stat))
          {
            info_message(UM_REPORT_CLASS, "SeQueryAuthenticationIdToken success");
          }
          else
          {
            error_message(UM_REPORT_CLASS, "SeQueryAuthenticationIdToken failed with status %!STATUS!", stat);
          }

          PsDereferencePrimaryToken(prim_token);
        }

        if (NT_SUCCESS(stat))
        {
          UNICODE_STRING* target_name(0);
          stat = support::query_target_file_for_rename_name(data, target_name);
          if (NT_SUCCESS(stat))
          {
            if (sizeof(um_rename_report.target_name) >= target_name->Length)
            {
              um_rename_report.target_name_size = target_name->Length;
              RtlCopyMemory(um_rename_report.target_name, target_name->Buffer, um_rename_report.target_name_size);
              info_message(UM_REPORT_CLASS, "name successfully copied to message buffer");
            }
            else
            {
              stat = STATUS_BUFFER_TOO_SMALL;
              error_message(UM_REPORT_CLASS, "message buffer too small for name");
            }

            ExFreePool(target_name);
          }
        }
      }
    }

    LONG64 get_report_number() const { return um_rename_report.report_number; }
    um_km_communication::rename_report* get_um_rename_report() { return &um_rename_report; }
  protected:
    um_km_communication::rename_report um_rename_report;
  };

  class report_with_um_section_handle : public report_with_um_report
  {
  public:
    report_with_um_section_handle(NTSTATUS& stat, PFLT_CALLBACK_DATA data) : report_with_um_report(stat, data)
    {
      if (NT_SUCCESS(stat))
      {
        KAPC_STATE apc_state;
        KeStackAttachProcess(reporter->get_eproc(), &apc_state);
        stat = ObOpenObjectByPointer(kernel_section_object, 0, 0, GENERIC_READ, *MmSectionObjectType, UserMode, &um_rename_report.section_handle);
        KeUnstackDetachProcess(&apc_state);
        if (NT_SUCCESS(stat))
        {
          info_message(UM_REPORT_CLASS, "ObOpenObjectByPointer success");
        }
        else
        {
          error_message(UM_REPORT_CLASS, "ObOpenObjectByPointer failed with status %!STATUS!", stat);
          um_rename_report.section_handle = 0;
        }

      }
    }

    ~report_with_um_section_handle()
    {
      if (reporter && um_rename_report.section_handle)
      {
        KAPC_STATE state;
        KeStackAttachProcess(reporter->get_eproc(), &state);

        KPROCESSOR_MODE old_mode = prev_mode_switcher::set_prev_mode(UserMode);
        NtClose(um_rename_report.section_handle);
        prev_mode_switcher::set_prev_mode(old_mode);

        KeUnstackDetachProcess(&state);
      }
    }
  };

  class top_report : public report_with_um_section_handle
  {
  public:
    top_report(NTSTATUS& stat, PFLT_CALLBACK_DATA data) : report_with_um_section_handle(stat, data)
    {}

    void* __cdecl operator new(size_t sz)
    {
      return ExAllocatePoolWithTag(NonPagedPool, sz, 'trpr');
    }
  };
}

void __cdecl um_report_class::report::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}

um_report_class::report* um_report_class::create_report(NTSTATUS& stat, PFLT_CALLBACK_DATA data)
{
  stat = STATUS_INSUFFICIENT_RESOURCES;
  um_report_class::report* r(new top_report(stat, data));
  if (NT_SUCCESS(stat))
  {
    info_message(UM_REPORT_CLASS, "new top_report success");
  }
  else
  {
    error_message(UM_REPORT_CLASS, "new top_report failed with status %!STATUS!", stat);
    delete r;
    r = 0;
  }

  return r;
}
