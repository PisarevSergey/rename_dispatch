#pragma once

namespace support
{
  template<typename T>
  class auto_flt_context
  {
  public:
    explicit auto_flt_context(void* c = 0) : ctx(static_cast<T*>(c))
    {}

    ~auto_flt_context()
    {
      if (ctx)
      {
        FltReleaseContext(ctx);
      }
    }

    T* release()
    {
      T* tmp(ctx);
      ctx = 0;
      return tmp;
    }

    void clear()
    {
      ctx = 0;
    }

    T* get()
    {
      return ctx;
    }

    T* operator->()
    {
      return ctx;
    }

    operator PFLT_CONTEXT()
    {
      return ctx;
    }

    operator PFLT_CONTEXT*()
    {
      return reinterpret_cast<PFLT_CONTEXT*>(&ctx);
    }

    operator bool() const { return (ctx ? true : false); }
  private:
    T* ctx;
  };

  template <typename T>
  class auto_pointer
  {
  public:
    explicit auto_pointer(T* p = 0) : ptr(p)
    {}

    ~auto_pointer()
    {
      if (ptr)
      {
        delete ptr;
      }
    }

    T* get()
    {
      return ptr;
    }

    T* operator->()
    {
      return ptr;
    }

    T* release()
    {
      T* tmp_ptr(ptr);

      ptr = 0;

      return tmp_ptr;
    }
    void clear()
    {
      ptr = 0;
    }
  private:
    T* ptr;
  };

  template <typename T>
  class list
  {
  public:
    list()
    {
      InitializeListHead(&head);
      ExInitializeFastMutex(&guard);
    }

    ~list()
    {
      while (T* e = pop())
      {
        delete e;
      }
    }

    void simple_push_unsafe(T* entry)
    {
      InsertTailList(&head, entry);
    }

    T* pop()
    {
      T* e(0);

      lock();

      if (FALSE == IsListEmpty(&head))
      {
        e = static_cast<T*>(RemoveHeadList(&head));
      }

      unlock();

      return e;
    }
  private:
    FAST_MUTEX guard;
  protected:
    LIST_ENTRY head;
    void lock() { ExAcquireFastMutex(&guard); }
    void unlock() { ExReleaseFastMutex(&guard); }
  };
}
