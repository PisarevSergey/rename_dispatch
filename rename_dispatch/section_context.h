#pragma once

namespace section_context
{
  struct context
  {
    ULONG dummy;
  };

  size_t get_size();
  context* create_context(NTSTATUS& stat);
}
