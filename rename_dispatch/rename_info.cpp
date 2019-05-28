#include "common.h"
#include "rename_info.tmh"

namespace
{
  class rename_info_with_thread : public rename_info::info
  {
  public:
    rename_info_with_thread(PETHREAD t) : thread(t) {}

    PETHREAD get_thread() const { return thread; }

  private:
    PETHREAD thread;
  };

  class rename_info_with_replace_if_exists_flag_tracking : public rename_info_with_thread
  {
  public:
    rename_info_with_replace_if_exists_flag_tracking(PETHREAD t, bool replace_cleared) : rename_info_with_thread(t),
      replace_flag_cleared(replace_cleared)
    {}

    bool is_replace_flag_cleared() const { return replace_flag_cleared; }

  private:
    bool replace_flag_cleared;
  };

  class top_rename_info : public rename_info_with_replace_if_exists_flag_tracking
  {
  public:
    top_rename_info(PETHREAD t, bool replace_cleared) : rename_info_with_replace_if_exists_flag_tracking(t, replace_cleared)
    {}

    void* __cdecl operator new(size_t sz)
    {
      return ExAllocatePoolWithTag(PagedPool, sz, 'ineR');
    }
  };
}

rename_info::info::info()
{
  InitializeListHead(this);
}

void __cdecl rename_info::info::operator delete(void* p)
{
  if (p)
  {
    ExFreePool(p);
  }
}

rename_info::info* rename_info::create_info(NTSTATUS& stat, PETHREAD thread, bool replace_flag_cleared)
{
  auto i = new top_rename_info(thread, replace_flag_cleared);
  if (i)
  {
    info_message(RENAME_INFO, "rename info for thread %p allocated at %p", thread, i);
    stat = STATUS_SUCCESS;
  }
  else
  {
    error_message(RENAME_INFO, "failed to allocate rename info for thread %p", thread);
    stat = STATUS_INSUFFICIENT_RESOURCES;
  }

  return i;
}
