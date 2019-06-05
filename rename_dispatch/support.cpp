#include "common.h"

namespace
{
  //NTSTATUS open_target_file_for_rename(PFLT_CALLBACK_DATA data,
  //  FILE_OBJECT** target_file_object)
  //{
  //  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  //  FILE_RENAME_INFORMATION* ren_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
  //  if (ren_info->FileNameLength && ('\\' == ren_info->FileName[0])) //absolute path
  //  {
  //    HANDLE target_file_handle(0);

  //  }
  //  else if (ren_info->RootDirectory)  //relative to target dir
  //  {
  //  }
  //  else  //move in the same dir
  //  {
  //  }

  //  return stat;
  //}
}

support::auto_flt_handle::~auto_flt_handle()
{
  if (h)
  {
    FltClose(h);
    h = 0;
  }
}

//NTSTATUS support::read_target_file_for_rename(PFLT_CALLBACK_DATA data)
//{
//  ASSERT(IRP_MJ_SET_INFORMATION == data->Iopb->MajorFunction);
//  ASSERT((FileRenameInformation   == data->Iopb->Parameters.SetFileInformation.FileInformationClass) ||
//         (FileRenameInformationEx == data->Iopb->Parameters.SetFileInformation.FileInformationClass));
//
//  NTSTATUS stat(STATUS_UNSUCCESSFUL);
//
//  FILE_RENAME_INFORMATION* ren_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
//  if (ren_info->FileNameLength && ('\\' == ren_info->FileName[0])) //absolute path
//  {
//  }
//  else if (ren_info->RootDirectory)  //relative to target dir
//  {
//  }
//  else  //move in the same dir
//  {
//  }
//  
//
//  return stat;
//}
