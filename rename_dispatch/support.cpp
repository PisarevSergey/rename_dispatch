#include "common.h"
#include "support.tmh"

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
        info_message(SUPPORT, "rename infor contains absolute path");
        stat = STATUS_SUCCESS;
      }
      else if(data->Iopb->Parameters.SetFileInformation.ParentOfTarget)
      {
        if (0 == (FO_HANDLE_CREATED & data->Iopb->Parameters.SetFileInformation.ParentOfTarget->Flags))
        {
          error_message(SUPPORT, "handle is not created for file object, exiting");
          break;
        }
        info_message(SUPPORT, "handle is created for file object, continue");

        stat = ObOpenObjectByPointer(data->Iopb->Parameters.SetFileInformation.ParentOfTarget,
          OBJ_KERNEL_HANDLE,
          0,
          FILE_LIST_DIRECTORY | FILE_TRAVERSE,
          *IoFileObjectType,
          KernelMode,
          root_dir);
        if (!NT_SUCCESS(stat))
        {
          error_message(SUPPORT, "ObOpenObjectByPointer failed with status %!STATUS!", stat);
          root_dir.clear();
          break;
        }
        info_message(SUPPORT, "ObOpenObjectByPointer success");
      }
      else
      {
        error_message(SUPPORT, "not absolute path and not relative open, invalid operation");
        break;
      }

      if (!NT_SUCCESS(stat))
      {
        error_message(SUPPORT, "rename info parsing failed with status %!STATUS!", stat);
        break;
      }
      info_message(SUPPORT, "rename info parsing success");

      support::auto_flt_handle target_file_handle;
      support::auto_referenced_object<FILE_OBJECT> target_file_obj;

      UNICODE_STRING target_file_name;
      target_file_name.Length = target_file_name.MaximumLength = static_cast<USHORT>(ren_info->FileNameLength);
      target_file_name.Buffer = ren_info->FileName;
      info_message(SUPPORT, "target file name is %wZ", &target_file_name);

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
        0,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        0,
        0,
        IO_IGNORE_SHARE_ACCESS_CHECK);
      if (!NT_SUCCESS(stat))
      {
        error_message(SUPPORT, "FltCreateFileEx failed with status %!STATUS!", stat);
        target_file_handle.clear();
        target_file_obj.clear();
        break;
      }
      info_message(SUPPORT, "FltCreateFileEx success");
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
      error_message(SUPPORT, "open_target_file_for_rename failed with status %!STATUS!", stat);
      break;
    }
    info_message(SUPPORT, "open_target_file_for_rename success");

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
      error_message(SUPPORT, "FltReadFile failed with status %!STATUS!", stat);
      break;
    }
    info_message(SUPPORT, "FltReadFile success");

    info_message(REPORT_FILE_CONTENTS, "target file contents %x", data_from_file);

  } while (false);

  return stat;
}

NTSTATUS support::query_target_file_for_rename_name(PFLT_CALLBACK_DATA data, UNICODE_STRING*& target_name)
{
  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  do
  {
    FILE_RENAME_INFORMATION* ren_info(static_cast<FILE_RENAME_INFORMATION*>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));
    if (ren_info->FileNameLength && ('\\' == ren_info->FileName[0])) //absolute path
    {
      info_message(SUPPORT, "rename information contains absolute path");

      target_name = static_cast<UNICODE_STRING*>(ExAllocatePoolWithTag(PagedPool,
        sizeof(*target_name) + ren_info->FileNameLength,
        'antr'));
      if (!target_name)
      {
        error_message(SUPPORT, "no memory for file name");
        stat = STATUS_INSUFFICIENT_RESOURCES;
        break;
      }
      info_message(SUPPORT, "memory successfully allocated for file name");

      target_name->Length = target_name->MaximumLength = static_cast<USHORT>(ren_info->FileNameLength);
      target_name->Buffer = reinterpret_cast<wchar_t*>(target_name + 1);
      RtlCopyMemory(target_name->Buffer, ren_info->FileName, target_name->Length);

      info_message(SUPPORT, "target file name is %wZ", target_name);

      stat = STATUS_SUCCESS;
    }
    else if (data->Iopb->Parameters.SetFileInformation.ParentOfTarget)
    {
      if (0 == (FO_HANDLE_CREATED & data->Iopb->Parameters.SetFileInformation.ParentOfTarget->Flags))
      {
        error_message(SUPPORT, "handle is not created for file object, exiting");
        break;
      }
      info_message(SUPPORT, "handle is created for file object, continue");

      support::auto_handle root_dir;
      stat = ObOpenObjectByPointer(data->Iopb->Parameters.SetFileInformation.ParentOfTarget,
        OBJ_KERNEL_HANDLE,
        0,
        FILE_LIST_DIRECTORY | FILE_TRAVERSE,
        *IoFileObjectType,
        KernelMode,
        root_dir);
      if (!NT_SUCCESS(stat))
      {
        error_message(SUPPORT, "ObOpenObjectByPointer failed with status %!STATUS!", stat);
        root_dir.clear();
        break;
      }
      info_message(SUPPORT, "ObOpenObjectByPointer success");

      support::auto_flt_handle target_file_handle;
      support::auto_referenced_object<FILE_OBJECT> target_file_obj;

      UNICODE_STRING target_file_name;
      target_file_name.Length = target_file_name.MaximumLength = static_cast<USHORT>(ren_info->FileNameLength);
      target_file_name.Buffer = ren_info->FileName;
      info_message(SUPPORT, "target file name is %wZ", &target_file_name);

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
        0,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        0,
        0,
        IO_IGNORE_SHARE_ACCESS_CHECK);
      if (!NT_SUCCESS(stat))
      {
        error_message(SUPPORT, "FltCreateFileEx failed with status %!STATUS!", stat);
        target_file_handle.clear();
        target_file_obj.clear();
        break;
      }
      info_message(SUPPORT, "FltCreateFileEx success");

      ULONG target_object_name_information_size(sizeof(*target_name) + 1024);
      target_name = static_cast<UNICODE_STRING*>(ExAllocatePoolWithTag(PagedPool,
        target_object_name_information_size,
        'antr'));
      if (!target_name)
      {
        error_message(SUPPORT, "no memory for target file name");
        stat = STATUS_INSUFFICIENT_RESOURCES;
        break;
      }
      info_message(SUPPORT, "target file name memory allocation success");

      stat = ObQueryNameString(target_file_obj,
        reinterpret_cast<POBJECT_NAME_INFORMATION>(target_name),
        target_object_name_information_size,
        &target_object_name_information_size);
      if (!NT_SUCCESS(stat))
      {
        error_message(SUPPORT, "ObQueryNameString failed with status %!STATUS!", stat);
        ExFreePool(target_name);
        break;
      }
      info_message(SUPPORT, "target file name is %wZ", target_name);
    }
    else
    {
      error_message(SUPPORT, "not absolute path and not relative open, invalid operation");
      break;
    }
  } while (false);

  return stat;
}
