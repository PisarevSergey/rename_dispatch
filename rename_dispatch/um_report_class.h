#pragma once

namespace um_report_class
{
  class report : public LIST_ENTRY
  {
  public:
    virtual ~report() {}
    virtual LONG64 get_report_number()const = 0; // { return um_rename_report.report_number; }
    virtual um_km_communication::rename_report* get_um_rename_report() = 0;
    void __cdecl operator delete(void* p);
  };

  report* create_report(NTSTATUS& stat, PFLT_CALLBACK_DATA data);
}
