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

void lock_writes();
void unlock_writes();