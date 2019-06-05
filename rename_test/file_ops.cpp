#include "common.h"

extern "C"
{
  NTSTATUS
    NTAPI
    NtSetInformationFile(
      _In_ HANDLE FileHandle,
      _Out_ PIO_STATUS_BLOCK IoStatusBlock,
      _In_reads_bytes_(Length) PVOID FileInformation,
      _In_ ULONG Length,
      _In_ int FileInformationClass
    );

}

namespace
{
  DWORD create_file_relative_to_dir(HANDLE* opened_file,
    HANDLE dir,
    const wchar_t* file_name,
    bool create_file) //true - create file, false - create directory
  {

    UNICODE_STRING file_name_us;
    RtlInitUnicodeString(&file_name_us, file_name);

    OBJECT_ATTRIBUTES obj_attrs;
    InitializeObjectAttributes(&obj_attrs, &file_name_us, OBJ_OPENIF, dir, 0);
    IO_STATUS_BLOCK iosb;
    NTSTATUS stat(NtCreateFile(opened_file,
      FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE | (create_file ? 0: (FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE)),
      &obj_attrs,
      &iosb,
      0,
      FILE_ATTRIBUTE_NORMAL,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      FILE_OPEN_IF,
      create_file ? FILE_NON_DIRECTORY_FILE : FILE_DIRECTORY_FILE,
      0,
      0));


    return RtlNtStatusToDosError(stat);
  }

  void rename_file_internal(HANDLE src_file,
    HANDLE target_dir,
    const wchar_t* target_file_name)
  {
    size_t target_file_name_size_in_bytes(sizeof(wchar_t) * wcslen(target_file_name));

    FILE_RENAME_INFO* ren_info(0);
    size_t rename_info_size = sizeof(*ren_info) + target_file_name_size_in_bytes;

    ren_info = static_cast<FILE_RENAME_INFO*>(malloc(rename_info_size));
    if (ren_info)
    {
      memset(ren_info, 0x0, rename_info_size);

      ren_info->ReplaceIfExists = TRUE;
      ren_info->RootDirectory = target_dir;
      ren_info->FileNameLength = static_cast<DWORD>(target_file_name_size_in_bytes);
      memcpy(ren_info->FileName, target_file_name, target_file_name_size_in_bytes);

      IO_STATUS_BLOCK iosb;
      NTSTATUS stat(NtSetInformationFile(src_file,
        &iosb,
        ren_info,
        static_cast<DWORD>(rename_info_size),
        10));

      if (NT_SUCCESS(stat))
      {
        wcout << L"rename success" << endl;
      }
      else
      {
        wcout << L"rename failed with status" << stat <<L" and error "<<RtlNtStatusToDosError(stat)<< endl;
      }

      free(ren_info);
    }
  }
}

void file_ops::rename_file(const wchar_t* base_dir_full_path,
  const wchar_t* src_file_name,
  const wchar_t* target_dir_name,
  const wchar_t* target_file_name)
{
  do
  {
    if (!base_dir_full_path)
    {
      wcout << L"null base dir, exiting" << endl;
      break;
    }
    wcout << L"base dir path " << base_dir_full_path << endl;

    if (!src_file_name)
    {
      wcout << L"null src file name, exiting" << endl;
      break;
    }
    wcout << L"src file name is " << src_file_name << endl;

    if (!target_file_name)
    {
      wcout << L"null target file name, exiting" << endl;
      break;
    }
    wcout << L"target file name is " << target_file_name << endl;

    wcout << L"opening base directory " << base_dir_full_path << endl;
    support::auto_handle base_dir(CreateFileW(base_dir_full_path,
      FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY,
      FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ,
      0,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS,
      0));
    if (!base_dir)
    {
      wcout << L"failed to open base directory with error "<< GetLastError() << endl;
      break;
    }
    wcout << L"open base directory success" << endl;

    support::auto_handle src_file;
    DWORD error(create_file_relative_to_dir(src_file, base_dir, src_file_name, true));
    if (ERROR_SUCCESS != error)
    {
      wcout << L"failed to create source file " << src_file_name << L" in directory " << base_dir_full_path << endl;
      break;
    }
    wcout << L"src file " << src_file_name << L" create success in dir " << base_dir_full_path << endl;

    support::auto_handle target_dir(0);
    if (target_dir_name)
    {
      wcout << L"creating target directory " << target_dir_name << endl;
      error = create_file_relative_to_dir(target_dir, base_dir, target_dir_name, false);
      if (ERROR_SUCCESS != error)
      {
        wcout << L"falied to create target dir with error " << error << endl;
        break;
      }
      wcout << L"create target directory success" << endl;
    }
    else
    {
      wcout << L"rename in same directory" << endl;
    }

    rename_file_internal(src_file,
      target_dir,
      target_file_name);

  } while (false);
}
