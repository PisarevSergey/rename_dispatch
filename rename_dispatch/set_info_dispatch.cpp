#include "common.h"
#include "set_info_dispatch.tmh"

FLT_PREOP_CALLBACK_STATUS
set_info_dispatch::pre(_Inout_ PFLT_CALLBACK_DATA    data,
                       _In_    PCFLT_RELATED_OBJECTS,
                       _Out_   PVOID* /*completion_context*/)
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


    FILE_RENAME_INFORMATION* rename_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
    rename_info = rename_info;
    data->Iopb->Parameters.SetFileInformation.ReplaceIfExists;

  } while (false);


  return fs_stat;
}

FLT_POSTOP_CALLBACK_STATUS
set_info_dispatch::post(_Inout_  PFLT_CALLBACK_DATA       data,
                        _In_     PCFLT_RELATED_OBJECTS,
                        _In_opt_ PVOID                    /*completion_context*/,
                        _In_     FLT_POST_OPERATION_FLAGS flags)
{
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
  //FLT_IS_REISSUED_IO();

  return fs_stat;
}
