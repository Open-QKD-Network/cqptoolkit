#include "SecureErase.h"
#include <cstdint>

namespace cqp {
#if !defined(memset_s)
#if !defined(DOXYGEN)
    #include <cerrno>
#endif

    /// Copies the value ch (after conversion to unsigned char as if by (unsigned char)ch) into each of the first count characters of the object pointed to by dest.
    /// @see http://en.cppreference.com/w/c/string/byte/memset
    /// @details
    /// memset may be optimized away (under the as-if rules) if the object modified by this function is not accessed again for the rest of its lifetime (e.g. gcc bug 8537). For that reason, this function cannot be used to scrub memory (e.g. to fill an array that stored a password with zeroes).
    /// This optimization is prohibited for memset_s: it is guaranteed to perform the memory write. Third-party solutions for that include FreeBSD explicit_bzero or Microsoft SecureZeroMemory.
    /// @returns zero on success, non-zero on error. Also on error, if dest is not a null pointer and destsz is valid, writes destsz fill bytes ch to the destination array.
    /// @param dest Memory region to change
    /// @param destsz size of destination
    /// @param ch value to write to destination
    /// @param count number of bytes to write
    int memset_s(void *dest, size_t destsz, char ch, size_t count)
    {
        if (dest == nullptr) return EINVAL;
        if (destsz > SIZE_MAX) return EINVAL;
        if (count > destsz) return EINVAL;

        volatile char *p = static_cast<volatile char*>(dest);
        while (destsz-- && count--)
        {
            *p++ = ch;
        }

        return 0;
    }
#endif

    void SecureErase(void* data, unsigned int length)
    {
        using namespace std;
        memset_s(data, length, 0, length);
    }

} // namespace cqp
