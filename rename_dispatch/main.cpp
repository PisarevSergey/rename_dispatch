#include "common.h"
#include "main.tmh"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING)
{
  NTSTATUS stat(STATUS_UNSUCCESSFUL);
  driver* d(0);

  do
  {
    d = create_driver(stat, drv);
    if (!NT_SUCCESS(stat))
    {
      ASSERT(0 == d);
      break;
    }
    ASSERT(d);
    info_message(MAIN, "driver successfully created");

    stat = d->start_filtering();
    if (!NT_SUCCESS(stat))
    {
      error_message(MAIN, "failed to start filtering with status %!STATUS!", stat);
      break;
    }
    info_message(MAIN, "filtering started successfully");

  } while (false);

  if (!NT_SUCCESS(stat) && d)
  {
    error_message(MAIN, "driver initialization failed with status %!STATUS!", stat);
    delete d;
  }

  return stat;
}
