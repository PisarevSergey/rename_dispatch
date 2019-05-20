#pragma once

class driver
{
public:
  virtual NTSTATUS start_filtering() = 0;

  virtual ~driver() {}
  void __cdecl operator delete(void*) {}
};

driver* create_driver(NTSTATUS& stat, PDRIVER_OBJECT);
driver* get_driver();