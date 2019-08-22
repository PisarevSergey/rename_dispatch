#include "common.h"
#include "rename_reporting.tmh"

extern "C" POBJECT_TYPE* MmSectionObjectType;

namespace
{
  NTSTATUS init_rename_report(PFLT_CALLBACK_DATA data, um_km_communication::rename_report& rep)
  {
    rep.pid = PsGetProcessId(IoThreadToProcess(data->Thread));
    info_message(RENAME_REPORTING, "pid: %p", rep.pid);

    rep.tid = PsGetThreadId(data->Thread);
    info_message(RENAME_REPORTING, "tid: %p", rep.tid);

    KeQuerySystemTime(&rep.time);

    NTSTATUS stat(STATUS_UNSUCCESSFUL);

    BOOLEAN copy_on_open;
    BOOLEAN effective_only;
    SECURITY_IMPERSONATION_LEVEL imp_lvl;
    PACCESS_TOKEN imp_token = PsReferenceImpersonationToken(data->Thread, &copy_on_open, &effective_only, &imp_lvl);
    if (imp_token)
    {
      info_message(RENAME_REPORTING, "thread impersonating");
      stat = SeQueryAuthenticationIdToken(imp_token, &rep.auth_id);
      if (NT_SUCCESS(stat))
      {
        info_message(RENAME_REPORTING, "SeQueryAuthenticationIdToken success");
      }
      else
      {
        error_message(RENAME_REPORTING, "SeQueryAuthenticationIdToken failed with status %!STATUS!", stat);
      }

      PsDereferenceImpersonationToken(imp_token);
    }
    else
    {
      info_message(RENAME_REPORTING, "thread is not impersonating");
      PACCESS_TOKEN prim_token = PsReferencePrimaryToken(IoThreadToProcess(data->Thread));
      stat = SeQueryAuthenticationIdToken(prim_token, &rep.auth_id);
      if (NT_SUCCESS(stat))
      {
        info_message(RENAME_REPORTING, "SeQueryAuthenticationIdToken success");
      }
      else
      {
        error_message(RENAME_REPORTING, "SeQueryAuthenticationIdToken failed with status %!STATUS!", stat);
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
          error_message(RENAME_REPORTING, "message buffer too small for name");
        }

        ExFreePool(target_name);
      }
    }

    return stat;
  }

  void report_rename_to_um_with_attaching_to_process_reporter(um_km_communication::rename_report& rep,
    void* section_object,
    PEPROCESS rep_proc)
  {
    KAPC_STATE apc_state;
    KeStackAttachProcess(rep_proc, &apc_state);
    NTSTATUS stat = ObOpenObjectByPointer(section_object, 0, 0, GENERIC_READ, *MmSectionObjectType, UserMode, &rep.section_handle);
    KeUnstackDetachProcess(&apc_state);
    if (NT_SUCCESS(stat))
    {
      info_message(RENAME_REPORTING, "ObOpenObjectByPointer success");

      LARGE_INTEGER timeout;
      timeout.QuadPart = -100000000; //10 seconds
      stat = send_message_to_um(&rep, sizeof(rep), &timeout);
      info_message(RENAME_REPORTING, "send_message_to_um status %!STATUS!", stat);


      KeStackAttachProcess(rep_proc, &apc_state);
      NtClose(rep.section_handle);
      KeUnstackDetachProcess(&apc_state);

    }
    else
    {
      error_message(RENAME_REPORTING, "ObOpenObjectByPointer failed with status %!STATUS!", stat);
    }
  }

  void report_rename_to_um_with_section_creation(section_context::context* sec_ctx, PFLT_CALLBACK_DATA data, um_km_communication::rename_report& rep)
  {
    auto reporter_proc(get_driver()->get_reporter_proc_ref());
    if (reporter_proc)
    {
      info_message(RENAME_REPORTING, "process reporter is active");

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
        info_message(RENAME_REPORTING, "FltCreateSectionForDataScan success");

        report_rename_to_um_with_attaching_to_process_reporter(rep, section_object, reporter_proc->get_eproc());

        ZwClose(section_handle);
        ObDereferenceObject(section_object);
        FltCloseSectionForDataScan(sec_ctx);
      }
      else
      {
        error_message(RENAME_REPORTING, "FltCreateSectionForDataScan failed with status %!STATUS!", stat);
      }

      reporter_proc->release();
    }
    else
    {
      info_message(RENAME_REPORTING, "no process reporter present");
    }
  }
}

void rename_reporting::report_operation_to_um(PFLT_CALLBACK_DATA data)
{
  um_km_communication::rename_report rr = { 0 };

  NTSTATUS stat = init_rename_report(data, rr);

  if (NT_SUCCESS(stat))
  {
    info_message(RENAME_REPORTING, "init rename report success");

    auto section_ctx = section_context::create_context(stat);
    if (NT_SUCCESS(stat))
    {
      info_message(RENAME_REPORTING, "section_context::create_context success");

      report_rename_to_um_with_section_creation(section_ctx, data, rr);

      FltReleaseContext(section_ctx);
    }
    else
    {
      error_message(RENAME_REPORTING, "section_context::create_context failed with status %!STATUS!", stat);
    }
  }
  else
  {
    error_message(RENAME_REPORTING, "init rename report failed with status %!STATUS!", stat);
  }
}
