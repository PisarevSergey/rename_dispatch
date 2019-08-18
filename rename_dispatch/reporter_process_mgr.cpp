#include "common.h"
#include "reporter_process_mgr.tmh"

reporter_process_mgr::manager::manager() : current_reporter(0)
{
  ExInitializeFastMutex(&guard);
}

reporter_process_mgr::manager::~manager()
{
  ExAcquireFastMutex(&guard);

  auto tmp_proc(current_reporter);
  current_reporter = 0;

  ExReleaseFastMutex(&guard);

  delete tmp_proc;
}

NTSTATUS reporter_process_mgr::manager::set_new_reporter_process(HANDLE pid)
{
  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  auto tmp_proc(referenced_reporter_process::create_process(stat, pid));
  if (NT_SUCCESS(stat))
  {
    info_message(REPORTER_PROCESS_MGR, "create_process success");

    ExAcquireFastMutex(&guard);
    auto old_proc(current_reporter);
    current_reporter = tmp_proc;
    ExReleaseFastMutex(&guard);

    delete old_proc;
  }
  else
  {
    error_message(REPORTER_PROCESS_MGR, "create_process failed with status %!STATUS!", stat);
    delete tmp_proc;
  }

  return stat;
}

void reporter_process_mgr::manager::reset_reporter_process()
{
  info_message(REPORTER_PROCESS_MGR, "reseting reporter process");

  ExAcquireFastMutex(&guard);
  auto old_proc(current_reporter);
  current_reporter = 0;
  ExReleaseFastMutex(&guard);

  delete old_proc;

}

referenced_reporter_process::process* reporter_process_mgr::manager::get_reporter_ref()
{
  ExAcquireFastMutex(&guard);

  auto current(current_reporter);
  if (current)
  {
    info_message(REPORTER_PROCESS_MGR, "reporter present");

    if (current->acquire())
    {
      info_message(REPORTER_PROCESS_MGR, "current reporter acquired successfully");
    }
    else
    {
      current = 0;
      error_message(REPORTER_PROCESS_MGR, "failed to acquire current reporter");
    }
  }
  else
  {
    info_message(REPORTER_PROCESS_MGR, "no current reporter");
  }

  ExReleaseFastMutex(&guard);

  return current;
}
