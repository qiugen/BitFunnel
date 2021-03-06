//
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//

#include <stdint.h>
#include <stddef.h>

#if defined(_MSC_VER)
#include <sal.h>
#define BITFUNNEL_FALLTHROUGH __fallthrough
#elif defined(__clang__)
#define BITFUNNEL_FALLTHROUGH [[clang::fallthrough]]
#else
#define BITFUNNEL_FALLTHROUGH
#endif


namespace BitFunnel
{
    //*************************************************************************
    // MurmurHash2, 64-bit versions, by Austin Appleby
    // The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
    // and endian-ness issues if used across multiple platforms.
    //*************************************************************************

    // 64-bit hash for 64-bit platforms
    uint64_t MurmurHash64A(const void *key, size_t len, unsigned seed)
    {
        const uint64_t m = 0xc6a4a7935bd1e995;
        const int r = 47;
        uint64_t h = seed ^ (len * m);
        const uint64_t *data = static_cast<const uint64_t *>(key);
        const uint64_t *end = data + (len / 8);
        while (data != end)
        {
            uint64_t k = *data++;
            k *= m;
            k ^= k >> r;
            k *= m;
            h ^= k;
            h *= m;
        }
        const unsigned char *data2 = reinterpret_cast<const unsigned char *>(data);

        switch(len &7)
        {
        case 7: h ^= uint64_t(data2[6]) << 48;
            BITFUNNEL_FALLTHROUGH;
        case 6: h ^= uint64_t(data2[5]) << 40;
            BITFUNNEL_FALLTHROUGH;
        case 5: h ^= uint64_t(data2[4]) << 32;
            BITFUNNEL_FALLTHROUGH;
        case 4: h ^= uint64_t(data2[3]) << 24;
            BITFUNNEL_FALLTHROUGH;
        case 3: h ^= uint64_t(data2[2]) << 16;
            BITFUNNEL_FALLTHROUGH;
        case 2: h ^= uint64_t(data2[1]) << 8;
            BITFUNNEL_FALLTHROUGH;
        case 1: h ^= uint64_t(data2[0]);
            h *= m;
        }
        ;
        h ^= h >> r;
        h *= m;
        h ^= h >> r;
        return h;
    }
}
