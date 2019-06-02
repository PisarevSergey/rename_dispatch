#include "common.h"

namespace
{
  DWORD get_current_dir(std::wstring& cur_dir)
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

  //DWORD rename_file(HANDLE file, )

  const wchar_t new_file_name[] = L"dsfsdfsdf";

  void perform_rename_in_same_dir(const std::wstring& base_dir)
  {

  }

  void perform_rename_in_another_dir(const std::wstring& base_dir)
  {}

  void perform_rename(bool in_same_dir)
  {
    do
    {
      std::wstring current_dir;
      DWORD error(get_current_dir(current_dir));
      if (error != ERROR_SUCCESS)
      {
        wcout << L"get_current_dir failed with error " << error << endl;
        break;
      }
      wcout << L"get_current_dir success" << endl;
      wcout << L"current dir is " << current_dir.c_str() << endl;

      HANDLE current_dir_handle = CreateFileW();

      if (in_same_dir)
      {
        wcout << L"rename inside one dir" << endl;
        perform_rename_in_same_dir(current_dir);
      }
      else
      {
        wcout << L"rename to another dir" << endl;
        perform_rename_in_another_dir(current_dir);
      }

    } while (false);

  }
}

int wmain(int argc, wchar_t* argv[])
{
  DWORD error(ERROR_SUCCESS);

  do
  {
    if (argc < 2)
    {
      wcout << L"not enough arguments" << endl;
      break;
    }
    wcout << L"argument is " << argv[1] << endl;

    const wchar_t in_current_dir[] = L"current";
    const wchar_t in_another_dir[] = L"another";

    if (0 == wcscmp(in_current_dir, argv[1]))
    {
      wcout << L"rename in current directory" << endl;
      perform_rename(true);
    }
    else if (0 == wcscmp(in_another_dir, argv[1]))
    {
      wcout << L"rename to another dir" << endl;
      perform_rename(false);
    }
    else
    {
      wcout << L"invalid argument" << endl;
      wcout << L"valid arguments:" << endl;
      wcout << L"  " <<in_current_dir << endl;
      wcout << L"  " <<in_another_dir << endl;
    }

  } while (false);

  return error;
}
