#pragma once

namespace prev_mode_switcher
{
  NTSTATUS init_switcher();
  KPROCESSOR_MODE set_prev_mode(KPROCESSOR_MODE new_mode);
}
