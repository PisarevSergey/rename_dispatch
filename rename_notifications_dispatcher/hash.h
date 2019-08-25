#pragma once

namespace hash
{
  DWORD compute_sha1_for_section(HANDLE section, const LARGE_INTEGER& file_size, unsigned char* sha_hash, unsigned size_of_sha_hash);
  void print_hash(const unsigned char* hash_to_print, const unsigned size_of_hash);
}
