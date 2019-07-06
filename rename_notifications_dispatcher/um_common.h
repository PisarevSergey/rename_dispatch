#pragma once

#include <Windows.h>
#include <wincrypt.h>
#include <winternl.h>
#include <fltUser.h>
#include <process.h>
#include <stdlib.h>

#include <iostream>

using std::wcout;
using std::endl;

#include "..\common_includes\um_km_communication.h"

#include "worker_thread.h"
#include "hash.h"