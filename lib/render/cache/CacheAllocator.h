// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

//
//
#pragma once

#include "CacheDequeue.h"
#include <scene_rdl2/render/util/StrUtil.h>

#include <cstddef> // std::size_t
#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>
#include <memory> // std::allocator (pulls in the STL's internal proxy type on MSVC)

namespace scene_rdl2 {
namespace cache {

//
// This allocator is designed for renderPrep cache dequeue situation like following condition.
//  - Cache data file is mmapped (memory mapped by mmap()) and it's read-only
//  - This mmapped memory is accessed via CacheDequeue object
//  - Construct vector and others by this CacheAllocator and just set data address from mmapped memory.
//  - We would like to skip initialization because set address already has data and 
//    we should not initialize that info (actually this memory is read-only).
//  - Created vector(and others) are accessed by read-only
//  - If we don't use CacheDequeue mode, behave as a regular allocator.
//
template <typename T>
class CacheAllocator
{
public:
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_swap;
    typedef std::false_type is_always_equal;

    using value_type = T;

    CacheAllocator() noexcept :
        mCacheDequeue(nullptr)
    {}
    CacheAllocator(CacheDequeue *cacheDequeue) noexcept :
        mCacheDequeue(cacheDequeue)
    {}
    template <typename U> CacheAllocator(const CacheAllocator<U> &u) noexcept :
        // via the public accessor: CacheAllocator<U> is an unrelated type,
        // so its protected member is inaccessible here (MSVC's debug STL
        // actually instantiates this rebind ctor)
        mCacheDequeue(u.getCacheDequeue())
    {}

    // In cacheDequeue mode only the container's element storage may come out
    // of the mapped cache data. MSVC's debug STL additionally allocates an
    // internal bookkeeping object (_Container_proxy) through a rebound
    // allocator; serving that from the cache would consume payload bytes and
    // desynchronize the dequeue, so such allocations go to the heap.
    static constexpr bool sServeFromCache =
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
        !std::is_same<T, std::_Container_proxy>::value;
#else
        true;
#endif

    T *allocate(std::size_t n)
    {
        if (!mCacheDequeue || !sServeFromCache) {
            return static_cast<T *>(::operator new(sizeof(T) * n));
        }
        return reinterpret_cast<T *>(getMem(sizeof(T) * n));
    }
    void deallocate(T *p, std::size_t n) noexcept
    {
        if (!mCacheDequeue || !sServeFromCache) ::operator delete(p);
    }

    template <typename U, typename... Args>
    void construct(U *p, Args &&... args)
    {
        if (!mCacheDequeue || !sServeFromCache) {
            ::new(p) U(std::forward<Args>(args)...);
        } else {
            // disable construct() function when cacheDeq enable mode
        }
    }

    size_t max_size() const noexcept
    {
        if (!mCacheDequeue) {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }
        return mCacheDequeue->getRestSize() / sizeof(T);
    }

    std::string show() const
    {
        std::ostringstream ostr;
        ostr << "CacheAllocator (addr:0x" << std::hex << (uintptr_t)this << std::dec
             << ") {\n";
        if (!mCacheDequeue) {
            ostr << "  mCacheDequeue:(empty)\n";
        } else {
            ostr << str_util::addIndent(mCacheDequeue->show()) << '\n';
        }
        ostr << "}";
        return ostr.str();
    }

    CacheDequeue *getCacheDequeue() const { return mCacheDequeue; }

protected:
    CacheDequeue *mCacheDequeue;

    uintptr_t getMem(size_t size) { return (uintptr_t)mCacheDequeue->skipByteData(size); }
};

template <typename T, typename U>
bool operator ==(const CacheAllocator<T> &a, const CacheAllocator<U> &b)
{
    if (a.getCacheDequeue() == b.getCacheDequeue()) return true;
    if (!a.getCacheDequeue() || !b.getCacheDequeue()) return false;
    return a.getCacheDequeue()->isSameEncodedData(*(b.getCacheDequeue()));
}

template <typename T, typename U>
bool operator !=(const CacheAllocator<T> &a, const CacheAllocator<U> &b)
{
    return !(a == b);
}

} // namespace cache
} // namespace scene_rdl2

