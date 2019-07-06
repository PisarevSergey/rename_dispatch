#pragma once

namespace hash
{
  DWORD compute_sha1_for_file(HANDLE file, unsigned char* sha_hash, unsigned size_of_sha_hash);
}
