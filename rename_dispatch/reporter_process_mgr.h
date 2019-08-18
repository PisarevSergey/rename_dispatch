#pragma once

namespace reporter_process_mgr
{
  class manager
  {
  public:
    manager();
    ~manager();

    NTSTATUS set_new_reporter_process(HANDLE pid);
    referenced_reporter_process::process* get_reporter_ref();
  private:
    FAST_MUTEX guard;
    referenced_reporter_process::process* current_reporter;
  };
}
