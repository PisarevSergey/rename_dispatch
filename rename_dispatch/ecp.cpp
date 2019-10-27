#include "common.h"
#include "ecp.tmh"

namespace
{
  bool is_ecp_prefetch_open(PECP_LIST guids)
  {
    bool is_prefetch_open;

    NTSTATUS stat(FltFindExtraCreateParameter(get_driver()->get_filter(), guids, &GUID_ECP_PREFETCH_OPEN, 0, 0));
    if (NT_SUCCESS(stat))
    {
      info_message(ECP, "this is prefetch open");
      is_prefetch_open = true;
    }
    else
    {
      error_message(ECP, "FltFindExtraCreateParameter failed with status %!STATUS!", stat);
      is_prefetch_open = false;
    }

    return is_prefetch_open;
  }

  bool is_clfs_create_container(PECP_LIST guids)
  {
    bool is_clfs_create_container;

    NTSTATUS stat(FltFindExtraCreateParameter(get_driver()->get_filter(), guids, &ECP_TYPE_CLFS_CREATE_CONTAINER, 0, 0));
    if (NT_SUCCESS(stat))
    {
      info_message(ECP, "this is prefetch open");
      is_clfs_create_container = true;
    }
    else
    {
      error_message(ECP, "FltFindExtraCreateParameter failed with status %!STATUS!", stat);
      is_clfs_create_container = false;
    }

    return is_clfs_create_container;
  }

}

NTSTATUS ecp::skip_this_create(PFLT_CALLBACK_DATA data, bool& skip)
{
  ASSERT(IRP_MJ_CREATE == data->Iopb->MajorFunction);

  skip = false;

  PECP_LIST guid_list(0);
  NTSTATUS stat = FltGetEcpListFromCallbackData(get_driver()->get_filter(), data, &guid_list);
  if (NT_SUCCESS(stat) && guid_list)
  {
    info_message(ECP, "FltGetEcpListFromCallbackData success");
    skip = (is_ecp_prefetch_open(guid_list) || is_clfs_create_container(guid_list));
  }
  else
  {
    error_message(ECP, "FltGetEcpListFromCallbackData failed with status %!STATUS! or guid_list is null", stat);
  }

  return stat;
}
