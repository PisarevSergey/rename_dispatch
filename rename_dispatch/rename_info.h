#pragma once

namespace rename_info
{
  class info : public LIST_ENTRY
  {
  public:
    info();
    virtual ~info() {}

    virtual PETHREAD get_thread() const = 0;
    virtual bool is_replace_flag_cleared() const = 0;

    void __cdecl operator delete(void* p);
  };

  info* create_info(NTSTATUS& stat, PETHREAD thread, bool replace_flag_cleared);
}
