#include <Windows.h>
#include <winternl.h>
#include <fltUser.h>

#include <iostream>

using std::wcout;
using std::endl;

#include "..\common_includes\um_km_communication.h"

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

    um_km_communication::um_rename_report um_rr;
    res = FilterGetMessage(port,
      &um_rr.hdr,
      sizeof(um_rr.hdr) + sizeof(um_rr.ren_rep),
      0);
    if (S_OK == res)
    {
      wcout << L"FilterGetMessage success" << endl;

      wcout << L"reply size expected " << um_rr.hdr.ReplyLength << endl;
      wcout << L"message id is " << um_rr.hdr.MessageId << endl;

      wcout << L"pid is " << um_rr.ren_rep.pid << endl;
      wcout << L"tid is " << um_rr.ren_rep.tid << endl;

      um_km_communication::um_reply reply;

      reply.hdr.Status = 0;
      reply.hdr.MessageId = um_rr.hdr.MessageId;
      res = FilterReplyMessage(port, &reply.hdr, sizeof(reply.hdr) + sizeof(reply.reply));
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

    CloseHandle(port);
  }
  else
  {
    wcout << L"FilterConnectCommunicationPort failed with result "<< res << endl;
  }

  return 0;
}