#pragma once

namespace um_report_class
{
  class report : public LIST_ENTRY
  {
  public:
    report(NTSTATUS& stat, PFLT_CALLBACK_DATA data);
    ~report();

    LONG64 get_report_number()const { return um_rename_report.report_number; }

    void* __cdecl operator new(size_t sz);
    void __cdecl operator delete(void* p);
  public:
    referenced_reporter_process::process* reporter;
    um_km_communication::rename_report um_rename_report;
  };
}
