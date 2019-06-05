#include "common.h"

namespace
{
  //DWORD rename_file(HANDLE file, )


  //void perform_rename_in_same_dir(const std::wstring& base_dir)
  //{

  //}

  //void perform_rename_in_another_dir(const std::wstring& base_dir)
  //{}

  void perform_rename(bool in_same_dir)
  {
    do
    {
      std::wstring current_dir;
      DWORD error(support::get_current_dir(current_dir));
      if (error != ERROR_SUCCESS)
      {
        wcout << L"get_current_dir failed with error " << error << endl;
        break;
      }
      wcout << L"get_current_dir success" << endl;
      wcout << L"current dir is " << current_dir.c_str() << endl;

      const wchar_t src_file_name[] = L"test.txt";
      const wchar_t new_file_name[] = L"dsfsdfsdf";
      const wchar_t rename_target_dir_name[] = L"ren_target_dir";
      file_ops::rename_file(current_dir.c_str(),
        src_file_name,
        in_same_dir ? 0 : rename_target_dir_name,
        new_file_name);

    } while (false);

  }
}

int wmain(int argc, wchar_t* argv[])
{
  DWORD error(ERROR_SUCCESS);

  wcout << L"press button to contiune" << endl;
  char c;
  std::cin >> c;

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
