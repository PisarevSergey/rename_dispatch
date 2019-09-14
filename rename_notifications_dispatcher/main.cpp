#include "um_common.h"

namespace
{
  class writer_locker
  {
  public:
    writer_locker()
    {
      InitializeCriticalSection(&guard);
    }

    ~writer_locker()
    {
      DeleteCriticalSection(&guard);
    }

    void lock() { EnterCriticalSection(&guard); }
    void unlock() { LeaveCriticalSection(&guard); }
  private:
    CRITICAL_SECTION guard;
  };

  writer_locker wl;
}

void lock_writes() { wl.lock(); }
void unlock_writes() { wl.unlock(); }

int main()
{
  um_km_communication::connection_context cc = {0};
  cc.reporter_pid = GetCurrentProcessId();

  HANDLE communication_port_handle;
  HRESULT res = FilterConnectCommunicationPort(um_km_communication::communication_port_name,
                                               0,
                                               &cc,
                                               sizeof(cc),
                                               0,
                                               &communication_port_handle);
  if (S_OK == res)
  {
    wcout << L"FilterConnectCommunicationPort success" << endl;

    auto io_completion_port = CreateIoCompletionPort(communication_port_handle, 0, 0, number_of_worker_threads);
    DWORD error(io_completion_port ? ERROR_SUCCESS : GetLastError());
    if (ERROR_SUCCESS == error)
    {
      wcout << L"CreateIoCompletionPort success" << endl;

      driver_communication_thread::driver_message_overlapped* overlapped_driver_messages = new driver_communication_thread::driver_message_overlapped[number_of_worker_threads];

      driver_communication_thread::start_thread_context tc;
      tc.communication_port = communication_port_handle;
      tc.io_completion_port = io_completion_port;

      HANDLE worker_threads[number_of_worker_threads];
      memset(worker_threads, 0x0, sizeof(worker_threads));

      DWORD actual_number_of_worker_threads(0);
      for (DWORD i(0); i < number_of_worker_threads; ++i)
      {
        tc.starting_message_to_use = (overlapped_driver_messages + i);
        worker_threads[i] = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, driver_communication_thread::worker, &tc, 0, 0));
        if (worker_threads[i])
        {
          ++actual_number_of_worker_threads;
        }
        else
        {
          break;
        }
      }

      if (actual_number_of_worker_threads)
      {
        WaitForMultipleObjects(actual_number_of_worker_threads, worker_threads, TRUE, INFINITE);
      }

      delete[] overlapped_driver_messages;

      CloseHandle(io_completion_port);
    }
    else
    {
      wcout << L"CreateIoCompletionPort failed with error "<< error << endl;
    }

    //if (S_OK == res)
    //{
    //  lock_writes();
    //  wcout << L"FilterGetMessage success" << endl;
    //  unlock_writes();
    //  error = worker_thread_pool->queue_work(driver_communication_thread::worker, um_rr);
    //  if (ERROR_SUCCESS == error)
    //  {
    //    lock_writes();
    //    wcout << L"workitem successfully queued" << endl;
    //    unlock_writes();
    //  }
    //  else
    //  {
    //    lock_writes();
    //    wcout << L"failed to queue workitem" << endl;
    //    unlock_writes();
    //    um_km_communication::um_reply reply;
    //    reply.hdr.Status = 0xC0000001L;
    //    reply.hdr.MessageId = um_rr->hdr.MessageId;
    //    FilterReplyMessage(communication_port_handle, &reply.hdr, sizeof(reply.hdr) + sizeof(reply.reply));
    //    delete[] um_rr;
    //  }
    //}

    CloseHandle(communication_port_handle);
  }
  else
  {
    wcout << L"FilterConnectCommunicationPort failed with result "<< res << endl;
  }

  return 0;
}