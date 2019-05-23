#include "common.h"

namespace
{
  FLT_OPERATION_REGISTRATION ops[] =
  {
    {IRP_MJ_CREATE, 0, create_dispatch::pre, create_dispatch::post},
    {IRP_MJ_OPERATION_END}
  };
}

FLT_OPERATION_REGISTRATION* operations::get_operations()
{
  return ops;
}