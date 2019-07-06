#pragma once

namespace um_km_communication
{
  const wchar_t communication_port_name[] = L"\\rename_comport_name";

  struct rename_report
  {
    HANDLE pid;
    HANDLE tid;
    LUID auth_id;
    LARGE_INTEGER time;
    ULONG target_name_size;
    wchar_t target_name[512];
  };

  struct um_rename_report
  {
    FILTER_MESSAGE_HEADER hdr;
    rename_report ren_rep;
  };

  struct reply_from_um
  {
    ULONG dummy;
  };

  struct um_reply
  {
    FILTER_REPLY_HEADER hdr;
    reply_from_um reply;
  };
}
