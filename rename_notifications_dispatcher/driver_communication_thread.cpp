#include "um_common.h"

extern HANDLE communication_port_handle;

unsigned __stdcall driver_communication_thread::worker(void*)
{
  for (;;)
  {
    um_km_communication::um_rename_report um_rr = { 0 };
    auto res = FilterGetMessage(communication_port_handle,
      &um_rr.hdr,
      sizeof(um_rr.hdr) + sizeof(um_rr.ren_rep),
      0);

    lock_writes();
    if (S_OK == res)
    {
      wcout << L"FilterGetMessage success" << endl;

      reporter::report_file_rename(&um_rr.ren_rep);

      um_km_communication::um_reply reply;
      reply.hdr.Status = 0;
      reply.hdr.MessageId = um_rr.hdr.MessageId;
      res = FilterReplyMessage(communication_port_handle, &reply.hdr, sizeof(reply.hdr) + sizeof(reply.reply));
      if (S_OK == res)
      {
        wcout << L"FilterReplyMessage success" << endl;
      }
      else
      {
        wcout << L"FilterReplyMessage failed with result " << res << endl;
      }
    }
    else
    {
      wcout << L"FilterGetMessage failed with result " << res << endl;
    }

    unlock_writes();
  }


  return 0;
}
