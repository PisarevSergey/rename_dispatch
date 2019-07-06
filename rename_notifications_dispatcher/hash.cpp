#include "um_common.h"

namespace
{
  class crypto_context
  {
  public:
    crypto_context(DWORD& error) : provider(0)
    {
      if (CryptAcquireContextW(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
      {
        error = ERROR_SUCCESS;
        wcout << L"CryptAcquireContextW success" << endl;
      }
      else
      {
        provider = 0;
        error = GetLastError();
        wcout << L"CryptAcquireContextW failed with error " << error << endl;
      }
    }

    ~crypto_context()
    {
      if (provider)
      {
        CryptReleaseContext(provider, 0);
      }
    }
  protected:
    HCRYPTPROV provider;
  };

  class hash_provider : public crypto_context
  {
  public:
    hash_provider(DWORD& error) : crypto_context(error), hash_handle(0)
    {
      if (ERROR_SUCCESS == error)
      {
        if (CryptCreateHash(provider, CALG_SHA, 0, 0, &hash_handle))
        {
          error = ERROR_SUCCESS;
          wcout << L"CryptCreateHash success" << endl;
        }
        else
        {
          error = GetLastError();
          wcout << L"CryptCreateHash failed with error" << endl;
        }
      }
    }

    ~hash_provider()
    {
      if (hash_handle)
      {
        CryptDestroyHash(hash_handle);
      }
    }

    DWORD hash_data(const unsigned char* data, DWORD data_size)
    {
      DWORD error;
      if (CryptHashData(hash_handle, data, data_size, 0))
      {
        error = ERROR_SUCCESS;
        wcout << L"CryptHashData success" << endl;
      }
      else
      {
        error = GetLastError();
        wcout << L"CryptHashData failed with error " << error << endl;
      }

      return error;
    }

    DWORD get_hash(unsigned char* sha_hash, DWORD sha_hash_size)
    {
      DWORD error; 

      if (TRUE == CryptGetHashParam(hash_handle, HP_HASHVAL, sha_hash, &sha_hash_size, 0))
      {
        error = ERROR_SUCCESS;
        wcout << L"successfully got hash for file" << endl;
      }
      else
      {
        error = GetLastError();
        wcout << L"failed to get hash for file with error " << error << endl;
      }

      return error;
    }
  private:
    HCRYPTHASH hash_handle;
  };
}

DWORD hash::compute_sha1_for_file(HANDLE file, unsigned char* sha_hash, unsigned size_of_sha_hash)
{
  DWORD error = ERROR_SUCCESS;
 
  std::auto_ptr<hash_provider> hash_prov(new hash_provider(error));
  if (ERROR_SUCCESS == error)
  {
    wcout << L"hash provider successfully created" << endl;

    unsigned char data_buffer[4096];
    DWORD read_bytes(0);

    for(;;)
    {
      if (ReadFile(file, data_buffer, sizeof(data_buffer), &read_bytes, 0))
      {
        error = ERROR_SUCCESS;
        wcout << L"successfully read " << read_bytes << L" bytes from file" << endl;
      }
      else
      {
        error = GetLastError();
        wcout << L"failed to read file with error " << error << endl;
        break;
      }

      if (0 == read_bytes)
      {
        wcout << L"read success but 0 bytes, exiting" << endl;
        break;
      }

      error = hash_prov->hash_data(data_buffer, read_bytes);
      if (error == ERROR_SUCCESS)
      {
        wcout << L"successfully hashed data" << endl;
      }
      else
      {
        wcout << L"failed to hash data with error " << error << endl;
        break;
      }
    }

    if (ERROR_SUCCESS == error)
    {
      wcout << L"successfully computed hash for file" << endl;
      error = hash_prov->get_hash(sha_hash, size_of_sha_hash);

    }
    else
    {
      wcout << L"failed to compute hash for file with error " << error << endl;
    }
  }
  else
  {
    wcout << L"failed to create hash provider with error " << error << endl;
  }

  return error;
}
