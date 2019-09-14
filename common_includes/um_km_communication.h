#pragma once

namespace um_km_communication
{
  const wchar_t communication_port_name[] = L"\\rename_comport_name";

  struct connection_context
  {
    unsigned __int64 reporter_pid;
  };
  static_assert(sizeof(HANDLE) <= sizeof(connection_context::reporter_pid), "wrong size");

  struct rename_report //for FltSendMessage
  {
    LONG64 report_number;
    HANDLE pid;
    HANDLE tid;
    LUID auth_id;
    LARGE_INTEGER time;
    HANDLE section_handle;
    LARGE_INTEGER size_of_mapped_file;
    ULONG target_name_size;
    wchar_t target_name[512];
  };

  struct um_rename_report // for FilterGetMessage
  {
    FILTER_MESSAGE_HEADER hdr;
    rename_report ren_rep;
  };

  struct release_report_message //for FilterSendMessage and MessageNotifyCallback
  {
    LONG64 report_number_to_release;
  };
}
