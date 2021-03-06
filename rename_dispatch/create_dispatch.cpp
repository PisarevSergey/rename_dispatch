#include "common.h"
#include "create_dispatch.tmh"

FLT_PREOP_CALLBACK_STATUS
create_dispatch::pre(
  _Inout_ PFLT_CALLBACK_DATA    data,
  _In_    PCFLT_RELATED_OBJECTS ,
  _Out_   PVOID *)
{
  FLT_PREOP_CALLBACK_STATUS fs_stat(FLT_PREOP_SUCCESS_NO_CALLBACK);

  do
  {
    if (0 == (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & DELETE))
    {
      verbose_message(CREATE_DISPATCH, "file is not being opened with DELETE access, skipping");
      break;
    }
    verbose_message(CREATE_DISPATCH, "file is being opened with DELETE access, continue");

    if (FO_VOLUME_OPEN & data->Iopb->TargetFileObject->Flags)
    {
      verbose_message(CREATE_DISPATCH, "skipping volume open");
      break;
    }
    verbose_message(CREATE_DISPATCH, "not volume open, continue");

    if (SL_OPEN_TARGET_DIRECTORY & data->Iopb->OperationFlags)
    {
      verbose_message(CREATE_DISPATCH, "skipping target dir open");
      break;
    }
    verbose_message(CREATE_DISPATCH, "not target dir open, continue");

    if (SL_OPEN_PAGING_FILE & data->Iopb->OperationFlags)
    {
      verbose_message(CREATE_DISPATCH, "skipping page file open");
      break;
    }
    verbose_message(CREATE_DISPATCH, "not page file open, continue");

    bool skip(false);
    if (NT_SUCCESS(ecp::skip_this_create(data, skip)) && skip)
    {
      verbose_message(CREATE_DISPATCH, "skipping for ECP reasons");
      break;
    }
    verbose_message(CREATE_DISPATCH, "not skipping by ECP, continue");

    fs_stat = FLT_PREOP_SUCCESS_WITH_CALLBACK;
  } while (false);

  return fs_stat;
}

FLT_POSTOP_CALLBACK_STATUS
create_dispatch::post(
  _Inout_  PFLT_CALLBACK_DATA       data,
  _In_     PCFLT_RELATED_OBJECTS    ,
  _In_opt_ PVOID                    ,
  _In_     FLT_POST_OPERATION_FLAGS flags)
{
  do
  {
    if (FLTFL_POST_OPERATION_DRAINING & flags)
    {
      info_message(CREATE_DISPATCH, "filter draining, exiting");
      break;
    }
    info_message(CREATE_DISPATCH, "filter is not draining, continue");

    if (!NT_SUCCESS(data->IoStatus.Status))
    {
      info_message(CREATE_DISPATCH, "create failed with status %!STATUS!, skipping", data->IoStatus.Status);
      break;
    }
    info_message(CREATE_DISPATCH, "create successful");

    if (STATUS_REPARSE == data->IoStatus.Status)
    {
      info_message(CREATE_DISPATCH, "reparse point, skipping");
      break;
    }

    BOOLEAN is_dir;
    NTSTATUS stat(FltIsDirectory(data->Iopb->TargetFileObject, data->Iopb->TargetInstance, &is_dir));
    if (!NT_SUCCESS(stat))
    {
      error_message(CREATE_DISPATCH, "FltIsDirectory failed with status %!STATUS!", stat);
      break;
    }
    info_message(CREATE_DISPATCH, "FltIsDirectory success");

    if (TRUE == is_dir)
    {
      info_message(CREATE_DISPATCH, "this is directory, skipping");
      break;
    }
    info_message(CREATE_DISPATCH, "this is regular file, continue");

    support::auto_flt_context<stream_context::context> sc(stream_context::create_context(stat));
    if (!NT_SUCCESS(stat))
    {
      error_message(CREATE_DISPATCH, "stream context creation failed with status %!STATUS!", stat);
      break;
    }
    info_message(CREATE_DISPATCH, "stream context creation success");

    stat = FltSetStreamContext(data->Iopb->TargetInstance,
                               data->Iopb->TargetFileObject,
                               FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                               sc,
                               0);
    if (!NT_SUCCESS(stat))
    {
      error_message(CREATE_DISPATCH, "FltSetStreamContext failed with status %!STATUS!", stat);
      break;
    }
    info_message(CREATE_DISPATCH, "FltSetStreamContext success");

  } while (false);

  return FLT_POSTOP_FINISHED_PROCESSING;
}
