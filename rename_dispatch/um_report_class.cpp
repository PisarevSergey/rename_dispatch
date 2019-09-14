#include "common.h"
#include "um_report_class.tmh"

extern "C" POBJECT_TYPE* MmSectionObjectType;

namespace
{
  NTSTATUS init_section_handle_with_attaching_to_process_reporter(um_km_communication::rename_report& rep,
    void* section_object,
    PEPROCESS rep_proc)
  {
    KAPC_STATE apc_state;
    KeStackAttachProcess(rep_proc, &apc_state);
    NTSTATUS stat = ObOpenObjectByPointer(section_object, 0, 0, GENERIC_READ, *MmSectionObjectType, UserMode, &rep.section_handle);
    KeUnstackDetachProcess(&apc_state);
    if (NT_SUCCESS(stat))
    {
      info_message(UM_REPORT_CLASS, "ObOpenObjectByPointer success");
    }
    else
    {
      error_message(UM_REPORT_CLASS, "ObOpenObjectByPointer failed with status %!STATUS!", stat);
      rep.section_handle = 0;
    }

    return stat;
  }

  NTSTATUS initialize_section_handle_with_section_creation(section_context::context* sec_ctx,
    PFLT_CALLBACK_DATA data,
    PEPROCESS reporter_proc,
    um_km_communication::rename_report& rep)
  {
    ASSERT(reporter_proc);

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, 0, OBJ_KERNEL_HANDLE, 0, 0);

    HANDLE section_handle;
    void* section_object;
    NTSTATUS stat = FltCreateSectionForDataScan(data->Iopb->TargetInstance,
      data->Iopb->TargetFileObject,
      sec_ctx,
      SECTION_MAP_READ,
      &oa,
      0,
      PAGE_READONLY,
      SEC_COMMIT,
      0,
      &section_handle,
      &section_object,
      &rep.size_of_mapped_file);
    if (NT_SUCCESS(stat))
    {
      info_message(UM_REPORT_CLASS, "FltCreateSectionForDataScan success");

      stat = init_section_handle_with_attaching_to_process_reporter(rep, section_object, reporter_proc);
      if (NT_SUCCESS(stat))
      {
        info_message(UM_REPORT_CLASS, "init_section_handle_with_attaching_to_process_reporter success");
      }
      else
      {
        error_message(UM_REPORT_CLASS, "init_section_handle_with_attaching_to_process_reporter failed with status %!STATUS!", stat);
      }

      ZwClose(section_handle);
      ObDereferenceObject(section_object);
      FltCloseSectionForDataScan(sec_ctx);
    }
    else
    {
      error_message(UM_REPORT_CLASS, "FltCreateSectionForDataScan failed with status %!STATUS!", stat);
    }

    return stat;
  }


  NTSTATUS initialize_section_handle(PFLT_CALLBACK_DATA data, PEPROCESS reporter_proc, um_km_communication::rename_report& rename_report)
  {
    NTSTATUS stat(STATUS_UNSUCCESSFUL);

    auto section_ctx = section_context::create_context(stat);
    if (NT_SUCCESS(stat))
    {
      info_message(UM_REPORT_CLASS, "section_context::create_context success");

      stat = initialize_section_handle_with_section_creation(section_ctx, data, reporter_proc, rename_report);

      FltReleaseContext(section_ctx);
    }
    else
    {
      error_message(UM_REPORT_CLASS, "section_context::create_context failed with status %!STATUS!", stat);
    }

    return stat;
  }

  NTSTATUS init_rename_report(PFLT_CALLBACK_DATA data, PEPROCESS reporter_proc, um_km_communication::rename_report& rep)
  {
    static LONG64 report_number(0);
    rep.report_number = InterlockedIncrement64(&report_number);

    rep.pid = PsGetProcessId(IoThreadToProcess(data->Thread));
    info_message(UM_REPORT_CLASS, "pid: %p", rep.pid);

    rep.tid = PsGetThreadId(data->Thread);
    info_message(UM_REPORT_CLASS, "tid: %p", rep.tid);

    KeQuerySystemTime(&rep.time);

    NTSTATUS stat(STATUS_UNSUCCESSFUL);

    BOOLEAN copy_on_open;
    BOOLEAN effective_only;
    SECURITY_IMPERSONATION_LEVEL imp_lvl;
    PACCESS_TOKEN imp_token = PsReferenceImpersonationToken(data->Thread, &copy_on_open, &effective_only, &imp_lvl);
    if (imp_token)
    {
      info_message(UM_REPORT_CLASS, "thread impersonating");
      stat = SeQueryAuthenticationIdToken(imp_token, &rep.auth_id);
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
      stat = SeQueryAuthenticationIdToken(prim_token, &rep.auth_id);
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
        if (sizeof(rep.target_name) >= target_name->Length)
        {
          rep.target_name_size = target_name->Length;
          RtlCopyMemory(rep.target_name, target_name->Buffer, rep.target_name_size);

        }
        else
        {
          stat = STATUS_BUFFER_TOO_SMALL;
          error_message(UM_REPORT_CLASS, "message buffer too small for name");
        }

        ExFreePool(target_name);
      }
    }

    if (NT_SUCCESS(stat))
    {
      stat = initialize_section_handle(data, reporter_proc, rep);
      if (NT_SUCCESS(stat))
      {
        info_message(UM_REPORT_CLASS, "initialize_section_handle success");
      }
      else
      {
        error_message(UM_REPORT_CLASS, "initialize_section_handle failed with status %!STATUS!", stat);
      }
    }

    return stat;
  }

}

um_report_class::report::report(NTSTATUS& stat, PFLT_CALLBACK_DATA data) : reporter(nullptr)
{
  do
  {
    this->Flink = this->Blink = 0;

    reporter = get_driver()->get_reporter_proc_ref();
    if (reporter)
    {
      info_message(UM_REPORT_CLASS, "reporter process active");
    }
    else
    {
      error_message(UM_REPORT_CLASS, "process-reporter absent");
      stat = STATUS_UNSUCCESSFUL;
      break;
    }

    stat = init_rename_report(data, reporter->get_eproc(), um_rename_report);
    if (NT_SUCCESS(stat))
    {
      info_message(UM_REPORT_CLASS, "init_rename_report success");
    }
    else
    {
      error_message(UM_REPORT_CLASS, "init_rename_report failed with status %!STATUS!", stat);
      break;
    }

  } while (false);

}

um_report_class::report::~report()
{
  if (reporter)
  {
    if (um_rename_report.section_handle)
    {
      KAPC_STATE state;
      KeStackAttachProcess(reporter->get_eproc(), &state);
      NtClose(um_rename_report.section_handle);
      KeUnstackDetachProcess(&state);
    }

    reporter->release();
  }
}

void* __cdecl um_report_class::report::operator new(size_t sz)
{
  return ExAllocatePoolWithTag(PagedPool, sz, 'trpr');
}

void __cdecl um_report_class::report::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}
