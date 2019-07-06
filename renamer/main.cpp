#include <Windows.h>
#include <iostream>

//#define _CRT_RAND_S
//#define _CRTBLD
//#include <stdlib.h>

using std::wcout;
using std::endl;
using std::wstring;

extern "C"
{
  errno_t rand_s(unsigned int* randomValue);
}

namespace
{
  wchar_t gen_random_letter()
  {
    static const wchar_t abc[] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','w','z' };
    unsigned int random_number;
    rand_s(&random_number);
    return abc[random_number % (sizeof(abc) / sizeof(abc[0]))];
  }

  void fill_random_null_terminated_string(wchar_t* buffer, size_t buffer_size_in_bytes)
  {
    const size_t buffer_size_in_chars(buffer_size_in_bytes / sizeof(buffer[0]));

    for (size_t i(0); i < buffer_size_in_chars; ++i)
    {
      if ((i + 1) == buffer_size_in_chars)
      {
        buffer[i] = 0;
      }
      else
      {
        buffer[i] = gen_random_letter();
      }
    }
  }

  DWORD create_and_fill_file_with_random_data(const wchar_t* file_path)
  {
    DWORD error(ERROR_SUCCESS);

    HANDLE file(CreateFileW(file_path, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0));
    if (INVALID_HANDLE_VALUE == file)
    {
      error = GetLastError();
      wcout << L"failed to create file " << file_path << L"with error " << error << endl;
    }
    else
    {
      wcout << L"file " << file_path << L" created successfully" << endl;

      static wchar_t data_buffer_to_write_to_file[800];
      fill_random_null_terminated_string(data_buffer_to_write_to_file, sizeof(data_buffer_to_write_to_file));

      DWORD written;
      if (WriteFile(file, data_buffer_to_write_to_file, sizeof(data_buffer_to_write_to_file), &written, 0))
      {
        wcout << L"file " << file_path << L" successfully filled with pattern" << endl;
      }
      else
      {
        error = GetLastError();
        wcout << L"failed to fill file " << file_path << L"with pattern" << endl;
      }

      CloseHandle(file);
    }

    return error;
  }
}

int wmain(int argc, wchar_t* argv[])
{
  DWORD error(ERROR_SUCCESS);

  do
  {
    if (argc < 2)
    {
      error = ERROR_INVALID_COMMAND_LINE;
      wcout << L"not enough arguments" << endl;
    }
    wcout << L"directory for moving files is " << argv[1] << endl;

    if (argv[1][wcslen(argv[1]) - 1] != '\\')
    {
      wcout << L"directory name not ending in \\" << endl;
      break;
    }
    wcout << L"directory name is ok" << endl;

    const wstring base_dir_name(argv[1]);

    for (;;)
    {
      Sleep(5000);

      wchar_t src_file_name[10];
      fill_random_null_terminated_string(src_file_name, sizeof(src_file_name));
      wcout << L"source file name is " << src_file_name << endl;

      const wstring src_file_path(base_dir_name + src_file_name);
      wcout << L"source file path is " << src_file_path.c_str() << endl;

      error = create_and_fill_file_with_random_data(src_file_path.c_str());
      if (ERROR_SUCCESS != error)
      {
        wcout << "failed to create and fill file " << src_file_path.c_str() << L" with error " << error << endl;
        continue;
      }
      wcout << L"successfully created and filled file " << src_file_path.c_str() << endl;

      wchar_t target_file_name[10];
      fill_random_null_terminated_string(target_file_name, sizeof(target_file_name));
      wcout << L"target file name is " << target_file_name << endl;

      const wstring target_file_path(base_dir_name + target_file_name);
      wcout << L"target file path is " << target_file_path.c_str() << endl;

      if (MoveFileW(src_file_path.c_str(), target_file_path.c_str()))
      {
        wcout << L"rename success" << endl;
      }
      else
      {
        error = GetLastError();
        wcout << L"rename failed with error " << error << endl;
      }
    }

  } while (false);

  return error;
}
