#pragma once

namespace section_notification_callback
{
  NTSTATUS callback(
    _In_ PFLT_INSTANCE      Instance,
    _In_ PFLT_CONTEXT       SectionContext,
    _In_ PFLT_CALLBACK_DATA Data
  );
}
