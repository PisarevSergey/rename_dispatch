#include "um_common.h"

int main()
{
  HANDLE port;
  HRESULT res = FilterConnectCommunicationPort(um_km_communication::communication_port_name,
                                               0,
                                               0,
                                               0,
                                               0,
                                               &port);
  if (S_OK == res)
  {
    wcout << L"FilterConnectCommunicationPort success" << endl;

    for (;;)
    {
      um_km_communication::um_rename_report um_rr = {0};
      res = FilterGetMessage(port,
        &um_rr.hdr,
        sizeof(um_rr.hdr) + sizeof(um_rr.ren_rep),
        0);
      if (S_OK == res)
      {
        wcout << L"FilterGetMessage success" << endl;

        um_km_communication::um_reply reply;
        reply.hdr.Status = 0;
        reply.hdr.MessageId = um_rr.hdr.MessageId;
        res = FilterReplyMessage(port, &reply.hdr, sizeof(reply.hdr) + sizeof(reply.reply));
        if (S_OK == res)
        {
          wcout << L"FilterReplyMessage success" << endl;

          um_km_communication::rename_report* report = reinterpret_cast<um_km_communication::rename_report*>(new char[sizeof(*report)]);
          *report = um_rr.ren_rep;
          worker_thread::report_file_rename_delayed(report);
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

    }

    CloseHandle(port);
  }
  else
  {
    wcout << L"FilterConnectCommunicationPort failed with result "<< res << endl;
  }

  return 0;
}