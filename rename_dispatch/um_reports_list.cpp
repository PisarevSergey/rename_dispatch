#include "common.h"
#include "um_reports_list.tmh"

namespace
{
  class list_with_guard : public um_reports_list::list
  {
  public:
    list_with_guard()
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

  class list_with_head : public list_with_guard
  {
  public:
    list_with_head()
    {
      InitializeListHead(&head);
    }

    ~list_with_head()
    {
      while (auto r = pop_report())
      {
        delete r;
      }
    }

    void push_report(um_report_class::report* r)
    {
      acquire_guard();

      InsertTailList(&head, r);

      release_guard();
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

    um_report_class::report* pop_report_by_number(const LONG64 number)
    {
      um_report_class::report* r(0);

      acquire_guard();

      for (um_report_class::report* cur_rep = static_cast<um_report_class::report*>(head.Flink); cur_rep != &head;)
      {
        um_report_class::report* next_rep = static_cast<um_report_class::report*>(cur_rep->Flink);

        if (cur_rep->get_report_number() == number)
        {
          r = cur_rep;
          RemoveEntryList(r);
          r->Flink = 0;
          r->Blink = 0;
          break;
        }

        cur_rep = next_rep;
      }

      release_guard();

      return r;
    }

    um_report_class::report* pop_old_report(const LONG age_in_seconds)
    {
      um_report_class::report* r(0);

      LARGE_INTEGER current_time;
      KeQuerySystemTime(&current_time);

      acquire_guard();

      for (um_report_class::report* cur_rep = static_cast<um_report_class::report*>(head.Flink); cur_rep != &head;)
      {
        um_report_class::report* next_rep = static_cast<um_report_class::report*>(cur_rep->Flink);

        if ((current_time.QuadPart - cur_rep->get_um_rename_report()->time.QuadPart) > support::seconds_to_100_nanosec_units(age_in_seconds).QuadPart)
        {
          r = cur_rep;
          RemoveEntryList(r);
          r->Flink = 0;
          r->Blink = 0;
          break;
        }

        cur_rep = next_rep;
      }

      release_guard();

      return r;
    }
  private:
    LIST_ENTRY head;
  };

  void cleaner_thread(void* list_ptr);

  class list_with_cleaner_thread : public list_with_head
  {
  public:
    list_with_cleaner_thread(NTSTATUS& stat) : thread_object(0), stop_thread(false)
    {
      HANDLE thread_handle(0);
      stat = PsCreateSystemThread(&thread_handle,
        SYNCHRONIZE,
        0,
        0,
        0,
        cleaner_thread,
        this);
      if (NT_SUCCESS(stat))
      {
        info_message(UM_REPORTS_LIST, "PsCreateSystemThread success");
        stat = ObReferenceObjectByHandle(thread_handle,
          SYNCHRONIZE,
          *PsThreadType,
          KernelMode,
          &thread_object,
          0);
        ASSERT(NT_SUCCESS(stat));
        ZwClose(thread_handle);
      }
      else
      {
        error_message(UM_REPORTS_LIST, "PsCreateSystemThread failed with status %!STATUS!", stat);
      }
    }

    ~list_with_cleaner_thread()
    {
      if (thread_object)
      {
        info_message(UM_REPORTS_LIST, "starting destruction");

        stop_thread = true;
        KeWaitForSingleObject(thread_object, Executive, KernelMode, FALSE, 0);
        ObDereferenceObject(thread_object);

        info_message(UM_REPORTS_LIST, "finished destruction");
      }
    }
  private:
    void* thread_object;
  public:
    bool stop_thread;
  };

  void cleaner_thread(void* list_ptr)
  {
    LARGE_INTEGER wake_interval = support::seconds_to_100_nanosec_units(1);

    while (false == static_cast<list_with_cleaner_thread*>(list_ptr)->stop_thread)
    {
      while (auto rep = static_cast<list_with_cleaner_thread*>(list_ptr)->pop_old_report(report_time_to_live_in_seconds))
      {
        delete rep;
      }

      KeDelayExecutionThread(KernelMode, FALSE, &wake_interval);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
  }


  class top_list : public list_with_cleaner_thread
  {
  public:
    top_list(NTSTATUS& stat) : list_with_cleaner_thread(stat)
    {}
  };
}

void* __cdecl um_reports_list::list::operator new(size_t sz)
{
  return ExAllocatePoolWithTag(NonPagedPool, sz, 'tslr');
}

void __cdecl um_reports_list::list::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}

um_reports_list::list* um_reports_list::create_list(NTSTATUS& stat)
{
  stat = STATUS_INSUFFICIENT_RESOURCES;
  um_reports_list::list* l = new top_list(stat);
  if (NT_SUCCESS(stat))
  {
    info_message(UM_REPORTS_LIST, "new list successfully created");
  }
  else
  {
    error_message(UM_REPORTS_LIST, "failed to create new list with status %!STATUS!", stat);
    delete l;
    l = 0;
  }

  return l;
}
