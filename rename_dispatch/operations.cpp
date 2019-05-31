#include "common.h"

namespace
{
  FLT_OPERATION_REGISTRATION ops[] =
  {
    {IRP_MJ_CREATE,          0,   create_dispatch::pre, create_dispatch::post},
    {IRP_MJ_SET_INFORMATION, 0, set_info_dispatch::pre, set_info_dispatch::post},
    {IRP_MJ_OPERATION_END}
  };
}

FLT_OPERATION_REGISTRATION* operations::get_registration()
{
  return ops;
}