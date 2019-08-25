#include "um_common.h"

namespace
{
  void print_system_time(const SYSTEMTIME* st)
  {
    wcout << st->wDay << L"." << st->wMonth << L"." << st->wYear << L" ";
    wcout << st->wHour << L":" << st->wMinute << endl;
  }
}

void reporter::report_file_rename(um_km_communication::rename_report* report)
{
  lock_writes();

  wcout << L"pid: " << report->pid << endl;
  wcout << L"tid: " << report->tid << endl;

  unsigned char sha_hash[20];
  DWORD error = hash::compute_sha1_for_section(report->section_handle, report->size_of_mapped_file, sha_hash, sizeof(sha_hash));
  if (ERROR_SUCCESS == error)
  {
    wcout << L"hash::compute_sha1_for_file success" << endl;

    //*************************************************************************
    wcout << L"****************************************************************" << endl;

    static_assert(sizeof(FILETIME) == sizeof(report->time), "wrong size");
    SYSTEMTIME event_time;
    FileTimeToSystemTime(reinterpret_cast<FILETIME*>(&report->time), &event_time);
    print_system_time(&event_time);

    std::wstring name(report->target_name, report->target_name_size / sizeof(wchar_t));
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

  unlock_writes();
}
