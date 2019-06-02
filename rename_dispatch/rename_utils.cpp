#include "common.h"
#include "rename_utils.tmh"

namespace
{
  NTSTATUS open_fltmgr_handle_for_parent_dir(PFLT_CALLBACK_DATA data,
    PFILE_OBJECT file_object,
    HANDLE& fltmgr_handle)
  {
    NTSTATUS stat(STATUS_UNSUCCESSFUL);

    do
    {
      ULONG name_buffer_size(0x400);
      support::auto_pool_allocation<OBJECT_NAME_INFORMATION> object_name;

      do
      {
        object_name.reset(ExAllocatePoolWithTag(PagedPool, name_buffer_size, 'inbO'));
        if (!object_name.get())
        {
          error_message(RENAME_UTILS, "failed to allocate object name buffer");
          stat = STATUS_INSUFFICIENT_RESOURCES;
          break;
        }
        verbose_message(RENAME_UTILS, "object name buffer allocation success");

        stat = ObQueryNameString(file_object,
          object_name.get(),
          name_buffer_size,
          &name_buffer_size);
        if (NT_SUCCESS(stat))
        {
          info_message(RENAME_UTILS, "ObQueryNameString success, object name is %wZ", &object_name->Name);
        }
        else
        {
          error_message(RENAME_UTILS, "ObQueryNameString failed with status %!STATUS!", stat);
        }

      } while (STATUS_INFO_LENGTH_MISMATCH == stat);

      if (!NT_SUCCESS(stat))
      {
        break;
      }

      OBJECT_ATTRIBUTES obj_attrs;
      InitializeObjectAttributes(&obj_attrs,
        &object_name->Name,
        OBJ_OPENIF | OBJ_KERNEL_HANDLE,
        0,
        0);
      IO_STATUS_BLOCK iosb;
      stat = FltCreateFile(get_driver()->get_filter(),
        data->Iopb->TargetInstance,
        &fltmgr_handle,
        FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
        &obj_attrs,
        &iosb,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE,
        0,
        0,
        0);
      if (NT_SUCCESS(stat))
      {
        info_message(RENAME_UTILS, "parent dir open success");
      }
      else
      {
        error_message(RENAME_UTILS, "parent dir open failed with status %!STATUS!", stat);
        fltmgr_handle = 0;
      }

    } while (false);

    return stat;
  }
}

void rename_utils::print_target_file_for_rename(PFLT_CALLBACK_DATA data)
{
  ASSERT(IRP_MJ_SET_INFORMATION == data->Iopb->MajorFunction);
  ASSERT((FileRenameInformation   == data->Iopb->Parameters.SetFileInformation.FileInformationClass) ||
         (FileRenameInformationEx == data->Iopb->Parameters.SetFileInformation.FileInformationClass));

  HANDLE parent_dir(0);

  //auto rename_info(static_cast<PFILE_RENAME_INFORMATION>(data->Iopb->Parameters.SetFileInformation.InfoBuffer));



  if (parent_dir)
  {
    FltClose(parent_dir);
  }
}
