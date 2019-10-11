#pragma once

namespace cleanup_dispatch
{
  FLT_PREOP_CALLBACK_STATUS pre(
    _Inout_ PFLT_CALLBACK_DATA    Data,
    _In_    PCFLT_RELATED_OBJECTS FltObjects,
    _Out_   PVOID* CompletionContext
    );
}
