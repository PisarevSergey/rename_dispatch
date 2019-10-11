#pragma once

#include <fltKernel.h>

#include "..\common_includes\um_km_communication.h"

#include "tracing.h"

#include "referenced_reporter_process.h"
#include "reporter_process_mgr.h"
#include "section_context.h"
#include "um_report_class.h"
#include "um_reports_list.h"
#include "reporter.h"
#include "driver.h"
#include "create_dispatch.h"
#include "operations.h"
#include "rename_info.h"
#include "stream_context.h"
#include "set_info_dispatch.h"
#include "support.h"
#include "prev_mode_switcher.h"
#include "delay_operation.h"

const LONG report_time_to_live_in_seconds = 10;