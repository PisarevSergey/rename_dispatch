#include "um_common.h"

driver_communication_thread::driver_message_overlapped::driver_message_overlapped()
{
  memset(&ol, 0x0, sizeof(ol));
  memset(&um_rr, 0x0, sizeof(um_rr));
}

unsigned __stdcall driver_communication_thread::worker(PVOID context)
{
  start_thread_context* tc = static_cast<start_thread_context*>(context);

  driver_communication_thread::driver_message_overlapped* message_with_overlapped = tc->starting_message_to_use;
  um_km_communication::um_rename_report* um_rr = &message_with_overlapped->um_rr;

  for(;;)
  {
    memset(um_rr, 0x00000000, sizeof(*um_rr));
    HRESULT result = FilterGetMessage(tc->communication_port,
      reinterpret_cast<PFILTER_MESSAGE_HEADER>(um_rr),
      sizeof(um_rr->hdr) + sizeof(um_rr->ren_rep),
      &message_with_overlapped->ol);
    if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) != result)
    {
      lock_writes();
      wcout << L"FilterGetMessage failed with result " << result << endl;
      unlock_writes();
      break;
    }
    lock_writes();
    wcout << L"FilterGetMessage successfully returned pending" << endl;
    unlock_writes();

    DWORD transfered(0);
    ULONG_PTR key(0);
    LPOVERLAPPED returned_ol;
    BOOL got_message(GetQueuedCompletionStatus(tc->io_completion_port, &transfered, &key, &returned_ol, INFINITE));
    if (FALSE == got_message)
    {
      DWORD error(GetLastError());

      lock_writes();
      wcout << L"GetQueuedCompletionStatus failed with error " << error << endl;
      unlock_writes();

      break;
    }

    lock_writes();
    wcout << L"GetQueuedCompletionStatus success" << endl;;
    unlock_writes();

    message_with_overlapped = reinterpret_cast<driver_communication_thread::driver_message_overlapped*>(returned_ol);
    um_rr = &message_with_overlapped->um_rr;

    lock_writes();
    reporter::report_file_rename(&um_rr->ren_rep);
    unlock_writes();

    um_km_communication::release_report_message release_msg = { 0 };
    release_msg.report_number_to_release = um_rr->ren_rep.report_number;
    DWORD returned(0);
    result = FilterSendMessage(tc->communication_port, &release_msg, sizeof(release_msg), 0, 0, &returned);
    if (S_OK == result)
    {
      lock_writes();
      wcout << L"FilterSendMessage with release message success" << endl;;
      unlock_writes();
    }
    else
    {
      lock_writes();
      wcout << L"FilterSendMessage with release message failed with result " << result << endl;;
      unlock_writes();
    }

    memset(message_with_overlapped, 0x0, sizeof(*message_with_overlapped));
  }

  return 0;
}
