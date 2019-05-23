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
    if (data->Iopb->Parameters.SetFileInformation.FileInformationClass != FileRenameInformation)
    {
      info_message(SET_INFO_DISPATCH, "this is not rename operation, skipping");
      break;
    }
    info_message(SET_INFO_DISPATCH, "this is rename operation");

    FILE_RENAME_INFORMATION* rename_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
    rename_info = rename_info;
    data->Iopb->Parameters.SetFileInformation.ReplaceIfExists;

  } while (false);


  return fs_stat;
}

FLT_POSTOP_CALLBACK_STATUS
set_info_dispatch::post(_Inout_  PFLT_CALLBACK_DATA       /*data*/,
                        _In_     PCFLT_RELATED_OBJECTS,
                        _In_opt_ PVOID                    /*completion_context*/,
                        _In_     FLT_POST_OPERATION_FLAGS /*flags*/)
{
  FLT_POSTOP_CALLBACK_STATUS fs_stat(FLT_POSTOP_FINISHED_PROCESSING);

  return fs_stat;
}
