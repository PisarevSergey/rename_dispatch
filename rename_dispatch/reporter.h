#pragma once

namespace reporter_facility
{
  class reporter
  {
  public:
    virtual ~reporter() {}

    virtual void push_report(um_report_class::report* r) = 0;
    virtual um_report_class::report* pop_report() = 0;

    virtual um_reports_list::list* get_list_waiting_for_cleaning() = 0;

    void* __cdecl operator new(size_t sz);
    void __cdecl operator delete(void* p);
  };

  reporter* create_reporter(NTSTATUS& stat);
}
