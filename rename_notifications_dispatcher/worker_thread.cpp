#include "um_common.h"

namespace
{
  void print_system_time(const SYSTEMTIME* st)
  {
    wcout << st->wDay << L"." << st->wMonth << L"." << st->wYear << L" ";
    wcout << st->wHour << L":" << st->wMinute << endl;
  }

  unsigned __stdcall reporter(void* report)
  {
    Sleep(10000);

    um_km_communication::rename_report* r(static_cast<um_km_communication::rename_report*>(report));

    lock_writes();

    wcout << L"pid: " << r->pid << endl;
    wcout << L"tid: " << r->tid << endl;



    HANDLE file(0);

    UNICODE_STRING file_name_us;
    file_name_us.Length = file_name_us.MaximumLength = static_cast<USHORT>(r->target_name_size);
    file_name_us.Buffer = r->target_name;

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa,
      &file_name_us,
      OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
      NULL,
      NULL);

    IO_STATUS_BLOCK iosb;
    NTSTATUS stat = NtOpenFile(&file,
      FILE_READ_DATA | SYNCHRONIZE,
      &oa,
      &iosb,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(stat))
    {
      wcout << "NtOpenFile success";

      unsigned char sha_hash[20];
      DWORD error = hash::compute_sha1_for_file(file, sha_hash, sizeof(sha_hash));
      if (ERROR_SUCCESS == error)
      {
        wcout << L"hash::compute_sha1_for_file success" << endl;

//*************************************************************************
        wcout << L"****************************************************************" << endl;

        static_assert(sizeof(FILETIME) == sizeof(r->time), "wrong size");
        SYSTEMTIME event_time;
        FileTimeToSystemTime(reinterpret_cast<FILETIME*>(&r->time), &event_time);
        print_system_time(&event_time);

        std::wstring name(r->target_name, r->target_name_size / sizeof(wchar_t));
        wcout << L"target file name is " << name.c_str() << endl;

        wcout << L"hash: ";
        hash::print_hash(sha_hash, sizeof(sha_hash));

        wcout << L"****************************************************************" << endl;

//*************************************************************************
      }
      else
      {
        wcout << L"hash::compute_sha1_for_file failed with error " << error << endl;
      }
      NtClose(file);
    }
    else
    {
      wcout << L"NtOpenFile failed with status " << stat << endl;
    }

    unlock_writes();

    delete[] report;
    return 0;
  }
}

void worker_thread::report_file_rename_delayed(um_km_communication::rename_report* report)
{
  if (_beginthreadex(0, 0, reporter, report, 0, 0))
  {
    wcout << L"thread creation success" << endl;
  }
  else
  {
    delete[] report;
    wcout << L"thread creation failed" << endl;
  }
}
