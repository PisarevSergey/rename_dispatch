#include "common.h"

support::auto_handle::~auto_handle()
{
  if ((h) && (h != INVALID_HANDLE_VALUE))
  {
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

void support::auto_handle::reset(HANDLE new_handle)
{
  if (h && (INVALID_HANDLE_VALUE != h) && (new_handle != h))
  {
    CloseHandle(h);
  }
  h = new_handle;
}

support::auto_handle::operator bool()
{
  return ((h) && (h != INVALID_HANDLE_VALUE)) ? true : false;
}

HANDLE support::auto_handle::release()
{
  HANDLE tmp = h;
  h = INVALID_HANDLE_VALUE;
  return tmp;
}

DWORD support::get_current_dir(std::wstring& cur_dir)
{
  DWORD error(ERROR_SUCCESS);

  const DWORD current_dir_path_length(MAX_PATH);
  wchar_t current_dir_path[current_dir_path_length];
  memset(current_dir_path, 0, sizeof(current_dir_path));

  DWORD number_of_chars_in_current_dir = GetCurrentDirectoryW(current_dir_path_length, current_dir_path);
  if (number_of_chars_in_current_dir)
  {
    wcout << L"current directory is " << current_dir_path << endl;
    cur_dir = current_dir_path;
  }
  else
  {
    error = GetLastError();
    wcout << L"GetCurrentDirectoryW failed with error " << error << endl;
  }

  return error;
}
