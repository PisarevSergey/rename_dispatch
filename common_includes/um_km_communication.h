#pragma once

namespace um_km_communication
{
  const wchar_t communication_port_name[] = L"\\rename_comport_name";

  struct rename_report
  {
    HANDLE pid;
    HANDLE tid;
    LUID auth_id;
    ULONG target_name_size;
    wchar_t target_name[512];
  };
}
