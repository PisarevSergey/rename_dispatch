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

HANDLE communication_port_handle;

void lock_writes() { wl.lock(); }
void unlock_writes() { wl.unlock(); }

int main()
{
  um_km_communication::connection_context cc = {0};
  cc.reporter_pid = GetCurrentProcessId();

  HRESULT res = FilterConnectCommunicationPort(um_km_communication::communication_port_name,
                                               0,
                                               &cc,
                                               sizeof(cc),
                                               0,
                                               &communication_port_handle);
  if (S_OK == res)
  {
    wcout << L"FilterConnectCommunicationPort success" << endl;

    const unsigned number_of_worker_threads_to_create(3);
    HANDLE worker_threads[number_of_worker_threads_to_create] = {0};
    unsigned actual_number_of_worker_threads(0);

    while (actual_number_of_worker_threads < number_of_worker_threads_to_create)
    {
      worker_threads[actual_number_of_worker_threads] = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, driver_communication_thread::worker, 0, 0, 0));
      if (worker_threads[actual_number_of_worker_threads])
      {
        wcout << L"thread number " << actual_number_of_worker_threads << L" created successfully" << endl;
        ++actual_number_of_worker_threads;
      }
      else
      {
        wcout << L"failed to start worker thread number "<< actual_number_of_worker_threads << endl;
        break;
      }
    }

    if (actual_number_of_worker_threads)
    {
      WaitForMultipleObjects(actual_number_of_worker_threads, worker_threads, TRUE, INFINITE);
    }

    CloseHandle(communication_port_handle);
  }
  else
  {
    wcout << L"FilterConnectCommunicationPort failed with result "<< res << endl;
  }

  return 0;
}