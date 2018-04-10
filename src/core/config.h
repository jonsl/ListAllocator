//
// Created by jslater on 10/03/18.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

namespace via {

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef float float32;
typedef double float64;
typedef int fd_handle_t;

}

#ifndef _VIA_DECL
#define _VIA_DECL// __attribute__((__visibility__("default")))
#endif

#ifndef _VIA_INLINE
#define _VIA_INLINE inline
#endif

#ifndef _VIA_NOEXCEPT
#define _VIA_NOEXCEPT _GLIBCXX_NOEXCEPT
#endif

#endif //CONFIG_H
