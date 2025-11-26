#pragma once
#include <cstdint>
#include <string>

// Optional: For WDF/DNP archive support
class C3HashSystem {
public:
    static uint32_t StringToID(const char* str);
    static uint32_t PackName(const char* path);
    static uint32_t RealName(const char* path);

private:
    static inline uint32_t RotateLeft(uint32_t value, int shift) {
        return (value << shift) | (value >> (32 - shift));
    }
};