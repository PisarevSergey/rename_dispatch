#include "common.h"
#include "reporter.tmh"

namespace
{
  class reporter_with_guard : public reporter_facility::reporter
  {
  public:
    reporter_with_guard()
    {
      ExInitializeFastMutex(&guard);
    }
  protected:
    void acquire_guard()
    {
      ExAcquireFastMutex(&guard);
    }

    void release_guard()
    {
      ExReleaseFastMutex(&guard);
    }
  private:
    FAST_MUTEX guard;
  };

  class reporter_with_reports_list : public reporter_with_guard
  {
  public:
    reporter_with_reports_list()
    {
      InitializeListHead(&head);
    }

    ~reporter_with_reports_list()
    {
      while (auto p = pop_report())
      {
        delete p;
      }
    }

    um_report_class::report* pop_report()
    {
      um_report_class::report* r(nullptr);

      acquire_guard();

      if (FALSE == IsListEmpty(&head))
      {
        r = static_cast<um_report_class::report*>(RemoveHeadList(&head));
        ASSERT(r);
      }

      release_guard();

      return r;
    }

  protected:
    void push_report_no_alarm(um_report_class::report* r)
    {
      acquire_guard();
      InsertTailList(&head, r);
      release_guard();
    }

  private:
    LIST_ENTRY head;
  };

  class reporter_with_alarm : public reporter_with_reports_list
  {
  public:
    reporter_with_alarm()
    {
      KeInitializeSemaphore(&alarm, 0, MAXLONG);
      KeInitializeEvent(&stopper, NotificationEvent, FALSE);
    }

    void wait_for_alarm()
    {
      void* objs_to_wait[2] = {&alarm, &stopper};
      KeWaitForMultipleObjects(2, objs_to_wait, WaitAny, Executive, KernelMode, FALSE, 0, 0);
    }

    void raise_alarm()
    {
      KeReleaseSemaphore(&alarm, SEMAPHORE_INCREMENT, 1, FALSE);
    }

    void push_report(um_report_class::report* r)
    {
      push_report_no_alarm(r);
      raise_alarm();
    }
  private:
    KSEMAPHORE alarm;
  protected:
    KEVENT stopper;
  };

  class reporter_with_list_for_cleaning : public reporter_with_alarm
  {
  public:
    reporter_with_list_for_cleaning(NTSTATUS& stat) : list_for_cleaning(nullptr)
    {
      list_for_cleaning = um_reports_list::create_list(stat);
    }

    ~reporter_with_list_for_cleaning()
    {
      delete list_for_cleaning;
    }

    um_reports_list::list* get_list_waiting_for_cleaning()
    {
      return list_for_cleaning;
    }
  protected:
    um_reports_list::list* list_for_cleaning;
  };

  void reporter_worker(void*);

  class reporter_with_worker : public reporter_with_list_for_cleaning
  {
  public:
    reporter_with_worker(NTSTATUS& stat) : reporter_with_list_for_cleaning(stat), reporter_thread(0), stop_thread(false)
    {
      if (NT_SUCCESS(stat))
      {
        HANDLE worker_handle(0);
        stat = PsCreateSystemThread(&worker_handle,
          SYNCHRONIZE,
          0,
          0,
          0,
          reporter_worker,
          this);
        if (NT_SUCCESS(stat))
        {
          info_message(REPORTER, "PsCreateSystemThread success");
          stat = ObReferenceObjectByHandle(worker_handle,
            SYNCHRONIZE,
            *PsThreadType,
            KernelMode,
            &reporter_thread,
            0);
          ASSERT(NT_SUCCESS(stat));
          ZwClose(worker_handle);
        }
        else
        {
          error_message(REPORTER, "PsCreateSystemThread failed with status %!STATUS!", stat);
        }
      }
    }

    ~reporter_with_worker()
    {
      if (reporter_thread)
      {
        info_message(REPORTER, "starting destruction");

        stop_thread = true;
        KeSetEvent(&stopper, 0, FALSE);
        KeWaitForSingleObject(reporter_thread, Executive, KernelMode, FALSE, 0);
        ObDereferenceObject(reporter_thread);

        info_message(REPORTER, "destruction finished");
      }
    }
  private:
    void* reporter_thread;
  public:
    bool stop_thread;
  };

  void reporter_worker(void* r)
  {
    reporter_with_worker* rep = static_cast<reporter_with_worker*>(r);

    LARGE_INTEGER timeout = support::seconds_to_100_nanosec_units(-5);
    NTSTATUS stat;

    while (false == rep->stop_thread)
    {
      rep->wait_for_alarm();

      um_report_class::report* current_rep = rep->pop_report();
      if (current_rep)
      {
        stat = send_message_to_um(current_rep->get_um_rename_report(), sizeof(*current_rep->get_um_rename_report()), &timeout);
        if (NT_SUCCESS(stat))
        {
          info_message(REPORTER, "send_message_to_um success");
        }
        else
        {
          error_message(REPORTER, "send_message_to_um failed with status %!STATUS!", stat);
        }

        rep->get_list_waiting_for_cleaning()->push_report(current_rep);
      }

    }

    PsTerminateSystemThread(STATUS_SUCCESS);
  }

  class top_reporter : public reporter_with_worker
  {
  public:
    top_reporter(NTSTATUS& stat) : reporter_with_worker(stat)
    {}
  };
}

void* __cdecl reporter_facility::reporter::operator new(size_t sz)
{
  return ExAllocatePoolWithTag(NonPagedPool, sz, 'rrpr');
}

void __cdecl reporter_facility::reporter::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}

reporter_facility::reporter* reporter_facility::create_reporter(NTSTATUS& stat)
{
  stat = STATUS_INSUFFICIENT_RESOURCES;
  reporter_facility::reporter* r(new top_reporter(stat));
  if (NT_SUCCESS(stat))
  {
    info_message(REPORTER, "new reporter successfully created");
  }
  else
  {
    error_message(REPORTER, "failed to create new reporter with status %!STATUS!", stat);
    delete r;
    r = 0;
  }

  return r;
}
