#include "common.h"

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
      static_cast<section_context::context*>(sec_ctx)->wait_for_finished_work(data);
      FltReleaseContext(sec_ctx);
    }
  }
}

void delay_operation::do_delay(PFLT_CALLBACK_DATA data)
{
  bool make_delay(false);

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
      make_delay = true;
    }
  }
    break;
  }

  if (make_delay)
  {
    do_actual_delay(data);
  }

}
