//
// Created by jslater on 10/03/18.
//

#ifndef VIA_CONFIG_H
#define VIA_CONFIG_H

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <sys/event.h>
#else

#include <sys/epoll.h>
#include <netinet/in.h>

#endif

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <atomic>
#include <csignal>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <condition_variable>
#include <thread>
#include <future>
#include <mutex>
#include <memory>
#include <scoped_allocator>
#include <limits>
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <deque>

/// POD types.
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef float float32;
typedef double float64;

namespace via {

/// types

typedef int fd_handle_t;
static constexpr fd_handle_t invalid_fd_handle(-1);
typedef std::chrono::milliseconds::rep time_msec_t;
static constexpr time_msec_t invalid_time_msec(-1);
typedef fd_handle_t socket_t;
static constexpr int max_processes = 1024;

/// constants

#ifdef _PAGE_SIZE // override
static constexpr int page_size = _PAGE_SIZE;
#elif defined(PAGE_SIZE)
static constexpr int page_size = PAGE_SIZE;
#endif
#ifdef _LEVEL1_DCACHE_LINESIZE
static constexpr int level1_dcache_linesize = _LEVEL1_DCACHE_LINESIZE;
#endif
#ifdef _NPROCESSORS_ONLINE
static constexpr int nprocessors_online = _NPROCESSORS_ONLINE;
#endif

}

#if __GNUC__ >= 4
#   define _VIA_PUBLIC __attribute__ ((visibility ("default")))
#   define _VIA_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#   define _VIA_PUBLIC
#   define _VIA_LOCAL
#endif

#ifndef _VIA_INLINE
#define _VIA_INLINE inline
#endif

#ifndef _VIA_NOEXCEPT
#ifdef __APPLE__
#define _VIA_NOEXCEPT _NOEXCEPT
#else
#define _VIA_NOEXCEPT _GLIBCXX_NOEXCEPT
#endif
#endif

// api


#endif //VIA_CONFIG_H
