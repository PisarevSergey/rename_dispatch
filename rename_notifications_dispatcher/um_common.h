#pragma once

#include <Windows.h>
#include <wincrypt.h>
#include <winternl.h>
#include <fltUser.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>

using std::wcout;
using std::endl;

#include "..\common_includes\um_km_communication.h"

#include "reporter.h"
#include "hash.h"
#include "driver_communication_thread.h"

void lock_writes();
void unlock_writes();

const DWORD number_of_worker_threads(3);
static_assert(number_of_worker_threads <= MAXIMUM_WAIT_OBJECTS, "too many worker threads");