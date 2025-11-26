#include "C3HashSystem.h"
#include <cstring>
#include <cctype>

uint32_t C3HashSystem::StringToID(const char* str) {
    if (!str) return 0;

    constexpr uint32_t X0 = 0x37A8470E;
    constexpr uint32_t Y0 = 0x7758B42B;
    constexpr uint32_t W_CONST = 0x267B0B11;
    constexpr uint32_t A = 0x2040801;
    constexpr uint32_t B = 0x804021;
    constexpr uint32_t C = 0xBFEF7FDF;
    constexpr uint32_t D = 0x7DFEFBFF;
    constexpr uint32_t V_INIT = 0xF4FA8928;
    constexpr uint32_t SENTINEL1 = 0x9BE74448;
    constexpr uint32_t SENTINEL2 = 0x66F42C48;

    uint32_t m[70] = { 0 };
    strncpy(reinterpret_cast<char*>(m), str, 256);

    int i;
    for (i = 0; i < 256 / 4 && m[i]; i++);
    m[i++] = SENTINEL1;
    m[i++] = SENTINEL2;

    uint32_t v = V_INIT;
    uint32_t esi = X0;
    uint32_t edi = Y0;

    for (int ecx = 0; ecx < i; ecx++) {
        uint32_t w = W_CONST;
        v = RotateLeft(v, 1);
        w ^= v;

        uint32_t eax = m[ecx];
        esi ^= eax;
        edi ^= eax;

        uint32_t edx = w + edi;
        edx = (edx | A) & C;

        uint64_t mul64 = static_cast<uint64_t>(esi) * edx;
        eax = static_cast<uint32_t>(mul64);
        edx = static_cast<uint32_t>(mul64 >> 32);

        uint32_t carry = 0;
        eax += edx;
        if (eax < edx) carry = 1;
        eax += carry;

        esi = eax;

        edx = w + esi;
        edx = (edx | B) & D;

        mul64 = static_cast<uint64_t>(edi) * edx;
        eax = static_cast<uint32_t>(mul64);
        edx = static_cast<uint32_t>(mul64 >> 32);

        edx += edx;
        eax += edx;
        if (eax < edx) eax += 2;

        edi = eax;
    }

    v = esi ^ edi;
    return v;
}

uint32_t C3HashSystem::PackName(const char* path) {
    if (!path) return 0;

    std::string buffer;
    buffer.reserve(256);

    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            buffer += ".wdf";
            break;
        }
        buffer += std::tolower(path[i]);
    }

    if (buffer.empty()) return 0;
    return StringToID(buffer.c_str());
}

uint32_t C3HashSystem::RealName(const char* path) {
    if (!path) return 0;

    std::string normalized;
    normalized.reserve(256);

    for (int i = 0; path[i]; i++) {
        char ch = path[i];
        if (ch >= 'A' && ch <= 'Z') {
            normalized += (ch - 'A' + 'a');
        }
        else if (ch == '\\') {
            normalized += '/';
        }
        else {
            normalized += ch;
        }
    }

    return StringToID(normalized.c_str());
}