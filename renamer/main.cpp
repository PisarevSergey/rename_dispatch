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
    wchar_t random_letter(random_number % (sizeof(abc) / sizeof(abc[0])));
    return random_letter;
  }

  void fill_random_string(wchar_t* buffer, size_t buffer_size_in_bytes)
  {
    const size_t buffer_size_in_chars(buffer_size_in_bytes / sizeof(buffer[0]));

    for (size_t i(0); i < buffer_size_in_chars; ++i)
    {
      buffer[i] = gen_random_letter();
    }
  }
}

DWORD wmain(int argc, wchar_t* argv[])
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

    wstring base_dir_name(argv[1]);

    for (;;)
    {
      break;
    }

  } while (false);

  return error;
}
