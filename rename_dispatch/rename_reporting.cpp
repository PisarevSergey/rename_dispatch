#include "common.h"
#include "rename_reporting.tmh"

void rename_reporting::report_operation_to_um(PFLT_CALLBACK_DATA data)
{
  um_km_communication::rename_report rr = { 0 };

  rr.pid = PsGetProcessId(IoThreadToProcess(data->Thread));
  info_message(RENAME_REPORTING, "pid: %p", rr.pid);

  rr.tid = PsGetThreadId(data->Thread);
  info_message(RENAME_REPORTING, "tid: %p", rr.tid);

  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  BOOLEAN copy_on_open;
  BOOLEAN effective_only;
  SECURITY_IMPERSONATION_LEVEL imp_lvl;
  PACCESS_TOKEN imp_token = PsReferenceImpersonationToken(data->Thread, &copy_on_open, &effective_only, &imp_lvl);
  if (imp_token)
  {
    info_message(RENAME_REPORTING, "thread impersonating");
    stat = SeQueryAuthenticationIdToken(imp_token, &rr.auth_id);
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
    stat = SeQueryAuthenticationIdToken(prim_token, &rr.auth_id);
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
      if (sizeof(rr.target_name) >= target_name->Length)
      {
        rr.target_name_size = target_name->Length;
        RtlCopyMemory(rr.target_name, target_name->Buffer, rr.target_name_size);

        LARGE_INTEGER timeout;
        timeout.QuadPart = 10000000000; //10 seconds

        stat = send_message_to_um(&rr, sizeof(rr), &timeout);
        info_message(RENAME_REPORTING, "send_message_to_um status %!STATUS!", stat);
      }
      else
      {
        stat = STATUS_BUFFER_TOO_SMALL;
        error_message(RENAME_REPORTING, "message buffer too small for name");
      }


      ExFreePool(target_name);
    }
  }
}
