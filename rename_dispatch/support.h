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
  class auto_pool_allocation
  {
  public:
    explicit auto_pool_allocation(void* allocation = 0) : allocated_mem(static_cast<T*>(allocation))
    {}

    ~auto_pool_allocation()
    {
      if (allocated_mem)
      {
        ExFreePool(allocated_mem);
        allocated_mem = 0;
      }
    }

    void reset(void* new_allocation = 0)
    {
      if (allocated_mem)
      {
        ExFreePool(allocated_mem);
      }
      allocated_mem = static_cast<T*>(new_allocation);
    }

    T* get()
    {
      return allocated_mem;
    }

    T* operator->()
    {
      return allocated_mem;
    }
  private:
    T* allocated_mem;
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

  template <typename T>
  class referenced_object
  {
  public:
    referenced_object(T* o = 0) : object(0) {}

    ~referenced_object()
    {
      if (object)
      {
        ObDereferenceObject(object);
      }
    }

    T* release()
    {
      T* tmp(object);
      object = 0;

      return tmp;
    }

    void clear() { object = 0; }

    T* operator->() { return object; }

    operator T* () { return object; }
    operator T** () { return &object; }
  private:
    T* object;
  };

  //NTSTATUS read_target_file_for_rename(PFLT_CALLBACK_DATA data);
}
