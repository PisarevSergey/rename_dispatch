#pragma once

namespace support
{
  class auto_handle
  {
  public:
    explicit auto_handle(HANDLE hndl = INVALID_HANDLE_VALUE) : h(hndl) {}
    ~auto_handle();

    operator HANDLE() { return h; }
    operator HANDLE* () { return &h; }
    operator bool();
  private:
    HANDLE h;
  };
}
