#include "common.h"

namespace
{
  NTSTATUS open_target_file_for_rename(PFLT_CALLBACK_DATA data,
    FILE_OBJECT** output_target_file_object,
    HANDLE* output_target_file_handle)
  {
    NTSTATUS stat(STATUS_UNSUCCESSFUL);

    do
    {
      support::auto_handle root_dir;

      FILE_RENAME_INFORMATION* ren_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
      if (ren_info->FileNameLength && ('\\' == ren_info->FileName[0])) //absolute path
      {
        stat = STATUS_SUCCESS;
      }
      else if(data->Iopb->Parameters.SetFileInformation.ParentOfTarget)
      {
        if (0 == (FO_HANDLE_CREATED & data->Iopb->Parameters.SetFileInformation.ParentOfTarget->Flags))
        {
          break;
        }

        stat = ObOpenObjectByPointer(data->Iopb->Parameters.SetFileInformation.ParentOfTarget,
          OBJ_KERNEL_HANDLE,
          0,
          FILE_LIST_DIRECTORY | FILE_TRAVERSE,
          *IoFileObjectType,
          KernelMode,
          root_dir);
        if (!NT_SUCCESS(stat))
        {
          root_dir.clear();
          break;
        }
      }
      else
      {
        break;
      }

      if (!NT_SUCCESS(stat))
      {
        break;
      }

      support::auto_flt_handle target_file_handle;
      support::auto_referenced_object<FILE_OBJECT> target_file_obj;

      UNICODE_STRING target_file_name;
      target_file_name.Length = target_file_name.MaximumLength = static_cast<USHORT>(ren_info->FileNameLength);
      target_file_name.Buffer = ren_info->FileName;

      OBJECT_ATTRIBUTES oa;
      InitializeObjectAttributes(&oa,
        &target_file_name,
        OBJ_OPENIF | OBJ_KERNEL_HANDLE,
        root_dir,
        0);

      IO_STATUS_BLOCK iosb;
      stat = FltCreateFileEx(get_driver()->get_filter(),
        data->Iopb->TargetInstance,
        target_file_handle,
        target_file_obj,
        FILE_READ_DATA,
        &oa,
        &iosb,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        0,
        0,
        0);
      if (!NT_SUCCESS(stat))
      {
        target_file_handle.clear();
        target_file_obj.clear();
        break;
      }
      *output_target_file_object = target_file_obj.release();
      *output_target_file_handle = target_file_handle.release();

    } while (false);

    return stat;
  }
}

HANDLE support::auto_handle::release()
{
  HANDLE tmp(h);
  h = 0;

  return tmp;
}

support::auto_handle::~auto_handle()
{
  if (h)
  {
    ZwClose(h);
    h = 0;
  }
}

HANDLE support::auto_flt_handle::release()
{
  HANDLE tmp(h);
  h = 0;

  return tmp;
}


support::auto_flt_handle::~auto_flt_handle()
{
  if (h)
  {
    FltClose(h);
    h = 0;
  }
}

NTSTATUS support::read_target_file_for_rename(PFLT_CALLBACK_DATA data)
{
  ASSERT(IRP_MJ_SET_INFORMATION == data->Iopb->MajorFunction);
  ASSERT((FileRenameInformation   == data->Iopb->Parameters.SetFileInformation.FileInformationClass) ||
         (FileRenameInformationEx == data->Iopb->Parameters.SetFileInformation.FileInformationClass));

  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  do
  {
    support::auto_referenced_object<FILE_OBJECT> target_file_object;
    support::auto_flt_handle target_file_handle;
    stat = open_target_file_for_rename(data, target_file_object, target_file_handle);
    if (!NT_SUCCESS(stat))
    {
      break;
    }

    unsigned __int32 data_from_file(0);
    ULONG data_size(sizeof(data_from_file));
    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    stat = FltReadFile(data->Iopb->TargetInstance,
      target_file_object,
      &offset,
      data_size,
      &data_from_file,
      0,
      &data_size,
      0,
      0);
    if (!NT_SUCCESS(stat))
    {
      break;
    }

  } while (false);

  return stat;
}
