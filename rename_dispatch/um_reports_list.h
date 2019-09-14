#pragma once

namespace um_reports_list
{
  class list
  {
  public:
    virtual void push_report(um_report_class::report* r) = 0;
    virtual um_report_class::report* pop_report_by_number(const LONG64 number) = 0;

    virtual ~list() {}
    void* __cdecl operator new(size_t sz);
    void __cdecl operator delete(void* p);
  };

  um_reports_list::list* create_list(NTSTATUS& stat);
}
