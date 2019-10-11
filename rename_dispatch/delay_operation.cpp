#include "common.h"
#include "delay_operation.tmh"

namespace
{
  void do_actual_delay(PFLT_CALLBACK_DATA data)
  {
    PFLT_CONTEXT sec_ctx(0);
    NTSTATUS stat = FltGetSectionContext(data->Iopb->TargetInstance,
      data->Iopb->TargetFileObject,
      &sec_ctx);
    if (NT_SUCCESS(stat))
    {
      info_message(DELAY_OPERATION, "successfully got section context");
      verbose_message(DELAY_OPERATION, "starting wait for finished work");
      static_cast<section_context::context*>(sec_ctx)->wait_for_finished_work(data);
      verbose_message(DELAY_OPERATION, "wait for finished work complete");
      FltReleaseContext(sec_ctx);
    }
    else
    {
      error_message(DELAY_OPERATION, "failed to get section context with status %!STATUS!", stat);
    }
  }
}

void delay_operation::do_delay(PFLT_CALLBACK_DATA data)
{
  bool make_delay(false);

  verbose_message(DELAY_OPERATION, "this operation is %s", FltGetIrpName(data->Iopb->MajorFunction));

  switch (data->Iopb->MajorFunction)
  {
  case IRP_MJ_CLEANUP:
    make_delay = true;
    break;
  case IRP_MJ_SET_INFORMATION:
  {
    if ((FileRenameInformation   == data->Iopb->Parameters.SetFileInformation.FileInformationClass) ||
        (FileRenameInformationEx == data->Iopb->Parameters.SetFileInformation.FileInformationClass))
    {
      verbose_message(DELAY_OPERATION, "this is rename operation");
      make_delay = true;
    }
  }
    break;
  }

  if (make_delay)
  {
    info_message(DELAY_OPERATION, "actually need to wait");
    do_actual_delay(data);
  }

}
