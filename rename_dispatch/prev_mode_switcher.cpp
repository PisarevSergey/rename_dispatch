#include "common.h"
#include "prev_mode_switcher.tmh"

namespace
{
  struct
  {
    unsigned __int32 prev_mode_offset_in_thread;
  } switcher;
}

KPROCESSOR_MODE ss;

NTSTATUS prev_mode_switcher::init_switcher()
{
  NTSTATUS stat(STATUS_UNSUCCESSFUL);

  const unsigned char test_sequence[] = { 0x65, 0x48, 0x8B, 0x04, 0x25, 0x88, 0x01, 0x00, 0x00, 0x0F, 0xB6, 0x80 };
  if (sizeof(test_sequence) == RtlCompareMemory(ExGetPreviousMode, test_sequence, sizeof(test_sequence)))
  {
    RtlCopyMemory(&switcher.prev_mode_offset_in_thread, Add2Ptr(ExGetPreviousMode, sizeof(test_sequence)), sizeof(switcher.prev_mode_offset_in_thread));
    stat = STATUS_SUCCESS;
  }

  return stat;
}

KPROCESSOR_MODE prev_mode_switcher::set_prev_mode(KPROCESSOR_MODE new_mode)
{
  KPROCESSOR_MODE old_mode(ExGetPreviousMode());

  KPROCESSOR_MODE* proc_mode_ptr = static_cast<KPROCESSOR_MODE*>(Add2Ptr(KeGetCurrentThread(), switcher.prev_mode_offset_in_thread));
  *proc_mode_ptr = new_mode;

  return old_mode;
}