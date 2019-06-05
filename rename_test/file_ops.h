#pragma once

namespace file_ops
{
  void rename_file(const wchar_t* base_dir_full_path,
    const wchar_t* src_file_name,
    const wchar_t* target_dir_name,
    const wchar_t* target_file_name);
}
