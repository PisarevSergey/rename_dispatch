#include "common.h"

support::auto_handle::~auto_handle()
{
  if ((h) && (h != INVALID_HANDLE_VALUE))
  {
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

support::auto_handle::operator bool()
{
  return ((h) && (h != INVALID_HANDLE_VALUE)) ? true : false;
}
