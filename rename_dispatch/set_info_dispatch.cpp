#include "common.h"
#include "set_info_dispatch.tmh"

FLT_PREOP_CALLBACK_STATUS
set_info_dispatch::pre(_Inout_ PFLT_CALLBACK_DATA    data,
                       _In_    PCFLT_RELATED_OBJECTS,
                       _Out_   PVOID* completion_context)
{
  FLT_PREOP_CALLBACK_STATUS fs_stat(FLT_PREOP_SUCCESS_NO_CALLBACK);

  do
  {
    if ((data->Iopb->Parameters.SetFileInformation.FileInformationClass != FileRenameInformation) &&
        (data->Iopb->Parameters.SetFileInformation.FileInformationClass != FileRenameInformationEx))
    {
      info_message(SET_INFO_DISPATCH, "this is not rename operation, skipping");
      break;
    }
    info_message(SET_INFO_DISPATCH, "this is rename operation");

    if (FLT_IS_FASTIO_OPERATION(data))
    {
      info_message(SET_INFO_DISPATCH, "failing fast i/o, request irp-based operation");
      fs_stat = FLT_PREOP_DISALLOW_FASTIO;
      break;
    }
    ASSERT(FLT_IS_IRP_OPERATION(data));
    info_message(SET_INFO_DISPATCH, "this is irp-based operation, continue dispatch");

    support::auto_flt_context<stream_context::context> sc;
    NTSTATUS stat(FltGetStreamContext(data->Iopb->TargetInstance, data->Iopb->TargetFileObject, sc));
    if (!NT_SUCCESS(stat))
    {
      error_message(SET_INFO_DISPATCH, "FltGetStreamContext failed with status %!STATUS!", stat);
      break;
    }
    info_message(SET_INFO_DISPATCH, "FltGetStreamContext success");

    bool replace_if_exists_cleared(data->Iopb->Parameters.SetFileInformation.ReplaceIfExists ? true : false);

    auto ren_info = rename_info::create_info(stat,
      PsGetCurrentThread(),
      replace_if_exists_cleared);
    if (!NT_SUCCESS(stat))
    {
      error_message(SET_INFO_DISPATCH, "rename_info::create_info failed with status %!STATUS!", stat);
      break;
    }
    info_message(SET_INFO_DISPATCH, "rename_info::create_info success");

    if (replace_if_exists_cleared)
    {
      data->Iopb->Parameters.SetFileInformation.ReplaceIfExists = FALSE;

      FILE_RENAME_INFORMATION* rename_info_buffer(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));

      switch(data->Iopb->Parameters.SetFileInformation.FileInformationClass)
      {
        case FileRenameInformation:
          rename_info_buffer->ReplaceIfExists = FALSE;
          break;
        case FileRenameInformationEx:
          rename_info_buffer->Flags &= ~FILE_RENAME_REPLACE_IF_EXISTS;
          break;
        default:
          ASSERT(FALSE);
      };
    }

    sc->insert_rename_info(ren_info);
    *completion_context = sc.release();

    fs_stat = FLT_PREOP_SYNCHRONIZE;

  } while (false);


  return fs_stat;
}

FLT_POSTOP_CALLBACK_STATUS
set_info_dispatch::post(_Inout_  PFLT_CALLBACK_DATA       data,
                        _In_     PCFLT_RELATED_OBJECTS,
                        _In_opt_ PVOID                    completion_context,
                        _In_     FLT_POST_OPERATION_FLAGS flags)
{
  support::auto_flt_context<stream_context::context> sc(completion_context);

  FLT_POSTOP_CALLBACK_STATUS fs_stat(FLT_POSTOP_FINISHED_PROCESSING);

  do
  {
    if (FLTFL_POST_OPERATION_DRAINING & flags)
    {
      info_message(SET_INFO_DISPATCH, "filter unloading, skip processing");
      break;
    }
    info_message(SET_INFO_DISPATCH, "filter is not unloading, continue processing");

    if (TRUE == FLT_IS_REISSUED_IO(data))
    {
      info_message(SET_INFO_DISPATCH, "reissued i/o, skipping");
      break;
    }
    info_message(SET_INFO_DISPATCH, "not a reissued i/o, continue processing");

    //FLT_POSTOP_MORE_PROCESSING_REQUIRED

  } while (false);

  return fs_stat;
}
