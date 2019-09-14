#pragma once

namespace driver_communication_thread
{
  struct driver_message_overlapped
  {
    driver_message_overlapped();

    OVERLAPPED ol;
    um_km_communication::um_rename_report um_rr;
  };

  struct start_thread_context
  {
    HANDLE communication_port;
    HANDLE io_completion_port;
    driver_message_overlapped* starting_message_to_use;
  };

  unsigned __stdcall worker(PVOID Context);
}
