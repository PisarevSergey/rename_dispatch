#include "common.h"
#include "set_info_dispatch.tmh"

namespace
{
  struct rename_work_item_context
  {
    void* stream_context;
    rename_info::info* ren_info;
  };

  void
    post_rename_dispatch(__in PFLT_CALLBACK_DATA data,
                         __in_opt PVOID context)
  {
    ASSERT(context);
    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    rename_work_item_context* ren_wi_ctx(static_cast<rename_work_item_context*>(context));
    support::auto_pointer<rename_info::info> ren_info(ren_wi_ctx->ren_info);
    support::auto_flt_context<stream_context::context> sc(ren_wi_ctx->stream_context);

    NTSTATUS read_status(support::read_target_file_for_rename(data));
    if (NT_SUCCESS(read_status))
    {
      info_message(SET_INFO_DISPATCH, "read_target_file_for_rename success");
    }
    else
    {
      error_message(SET_INFO_DISPATCH, "read_target_file_for_rename failed with status %!STATUS!", read_status);
    }

    if (ren_info.get())
    {
      if (ren_info->is_replace_flag_cleared())
      {
        info_message(SET_INFO_DISPATCH, "restoring rename flag");
        data->Iopb->Parameters.SetFileInformation.ReplaceIfExists = TRUE;
        FILE_RENAME_INFORMATION* rename_info_buffer(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));

        switch (data->Iopb->Parameters.SetFileInformation.FileInformationClass)
        {
        case FileRenameInformation:
          rename_info_buffer->ReplaceIfExists = TRUE;
          break;
        case FileRenameInformationEx:
          rename_info_buffer->Flags |= FILE_RENAME_REPLACE_IF_EXISTS;
          break;
        default:
          ASSERT(FALSE);
        };

        FltSetCallbackDataDirty(data);

        info_message(SET_INFO_DISPATCH, "reissuing rename");
        FltReissueSynchronousIo(data->Iopb->TargetInstance, data);
        if (NT_SUCCESS(data->IoStatus.Status))
        {
          info_message(SET_INFO_DISPATCH, "reissued io successfully completed");
        }
        else
        {
          error_message(SET_INFO_DISPATCH, "reissued io failed with status %!STATUS!", data->IoStatus.Status);
        }
      }
    }
  }
}

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

    support::auto_flt_context<stream_context::context> sc;
    NTSTATUS stat(FltGetStreamContext(data->Iopb->TargetInstance, data->Iopb->TargetFileObject, sc));
    if (!NT_SUCCESS(stat))
    {
      error_message(SET_INFO_DISPATCH, "FltGetStreamContext failed with status %!STATUS!", stat);
      break;
    }
    info_message(SET_INFO_DISPATCH, "FltGetStreamContext success");

    if (FLT_IS_FASTIO_OPERATION(data))
    {
      info_message(SET_INFO_DISPATCH, "failing fast i/o, request irp-based operation");
      fs_stat = FLT_PREOP_DISALLOW_FASTIO;
      break;
    }
    ASSERT(FLT_IS_IRP_OPERATION(data));
    info_message(SET_INFO_DISPATCH, "this is irp-based operation, continue dispatch");

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
      info_message(SET_INFO_DISPATCH, "clearing replace if exists flag");
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

      FltSetCallbackDataDirty(data);
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

  support::auto_pointer<rename_info::info> ren_info(sc->extract_rename_info_by_thread(PsGetCurrentThread()));
  ASSERT(ren_info.get());

  FLT_POSTOP_CALLBACK_STATUS fs_stat(FLT_POSTOP_FINISHED_PROCESSING);

  do
  {
    if (FLTFL_POST_OPERATION_DRAINING & flags)
    {
      info_message(SET_INFO_DISPATCH, "filter unloading, skip processing");
      break;
    }
    info_message(SET_INFO_DISPATCH, "filter is not unloading, continue processing");


    if (data->IoStatus.Status != STATUS_OBJECT_NAME_COLLISION)
    {
      info_message(SET_INFO_DISPATCH, "final operation status is %!STATUS!, skipping", data->IoStatus.Status);

      if (NT_SUCCESS(data->IoStatus.Status))
      {
        NTSTATUS stat(STATUS_INSUFFICIENT_RESOURCES);
        auto report_for_um = um_report_class::create_report(stat, data);
        if (NT_SUCCESS(stat))
        {
          get_driver()->get_reporter()->push_report(report_for_um);
        }
        else
        {
          delete report_for_um;
        }
      }

      break;
    }
    info_message(SET_INFO_DISPATCH, "rename failed due to collision, dispatching");

    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    rename_work_item_context work_item_ctx;
    work_item_ctx.stream_context = sc.release();
    work_item_ctx.ren_info = ren_info.release();

    post_rename_dispatch(data, &work_item_ctx);

    NTSTATUS stat(STATUS_INSUFFICIENT_RESOURCES);
    auto report_for_um = um_report_class::create_report(stat, data);
    if (NT_SUCCESS(stat))
    {
      get_driver()->get_reporter()->push_report(report_for_um);
    }
    else
    {
      delete report_for_um;
    }

    fs_stat = FLT_POSTOP_FINISHED_PROCESSING;

  } while (false);

  return fs_stat;
}
