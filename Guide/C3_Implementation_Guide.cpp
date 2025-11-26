# Yamen Engine - C3 Asset System Implementation Guide
## Practical C++ Code for Modern Game Engine

---

## 📋 TABLE OF CONTENTS

1. Hash System Implementation
2. Archive Loaders (WDF/DNP)
3. Asset Base Classes
4. Format-Specific Loaders
5. Asset Manager
6. Example Usage
7. Testing & Validation

---

## 1. HASH SYSTEM IMPLEMENTATION

### C++ Port of stringtoid() Algorithm

```cpp
// File: Core/C3Hash.h
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>

namespace Yamen::C3 {

class HashSystem {
public:
    // Main hash function (port of stringtoid)
    static uint32_t StringToID(const char* str);
    
    // Extract archive name from path
    static uint32_t PackName(const char* path);
    
    // Extract file name from path  
    static uint32_t RealName(const char* path);
    
private:
    // Assembly operations ported to C++
    static inline uint32_t RotateLeft(uint32_t value, int shift) {
        return (value << shift) | (value >> (32 - shift));
    }
};

} // namespace Yamen::C3
```

```cpp
// File: Core/C3Hash.cpp
#include "C3Hash.h"
#include <cstring>
#include <cctype>

namespace Yamen::C3 {

uint32_t HashSystem::StringToID(const char* str) {
    if (!str) return 0;
    
    // Magic constants from original assembly
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
    
    // Copy string to buffer (max 256 chars)
    uint32_t m[70] = {0}; // 70 * 4 = 280 bytes > 256
    std::strncpy(reinterpret_cast<char*>(m), str, 256);
    
    // Find string length in DWORDs
    int i;
    for (i = 0; i < 256/4 && m[i]; i++);
    
    // Append sentinels
    m[i++] = SENTINEL1;
    m[i++] = SENTINEL2;
    
    // Initialize accumulators
    uint32_t v = V_INIT;
    uint32_t esi = X0;
    uint32_t edi = Y0;
    
    // Main loop
    for (int ecx = 0; ecx < i; ecx++) {
        uint32_t w = W_CONST;
        v = RotateLeft(v, 1);
        w ^= v;
        
        uint32_t eax = m[ecx];
        esi ^= eax;
        edi ^= eax;
        
        // First multiply-add-carry operation
        uint32_t edx = w + edi;
        edx = (edx | A) & C;
        
        uint64_t mul64 = static_cast<uint64_t>(esi) * edx;
        eax = static_cast<uint32_t>(mul64);
        edx = static_cast<uint32_t>(mul64 >> 32);
        
        uint32_t carry = 0;
        eax += edx; if (eax < edx) carry = 1;
        eax += carry;
        
        esi = eax;
        
        // Second multiply-add-carry operation
        edx = w + esi;
        edx = (edx | B) & D;
        
        mul64 = static_cast<uint64_t>(edi) * edx;
        eax = static_cast<uint32_t>(mul64);
        edx = static_cast<uint32_t>(mul64 >> 32);
        
        edx += edx;
        eax += edx;
        if (aex < edx) aex += 2; // Carry handling
        
        edi = eax;
    }
    
    v = esi ^ edi;
    return v;
}

uint32_t HashSystem::PackName(const char* path) {
    if (!path) return 0;
    
    std::string buffer;
    buffer.reserve(256);
    
    // Copy until first '/' and convert to lowercase
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

uint32_t HashSystem::RealName(const char* path) {
    if (!path) return 0;
    
    // Normalize: lowercase + forward slashes
    std::string normalized;
    normalized.reserve(256);
    
    for (int i = 0; path[i]; i++) {
        char ch = path[i];
        if (ch >= 'A' && ch <= 'Z') {
            normalized += (ch - 'A' + 'a');
        } else if (ch == '\\') {
            normalized += '/';
        } else {
            normalized += ch;
        }
    }
    
    return StringToID(normalized.c_str());
}

} // namespace Yamen::C3
```

---

## 2. ARCHIVE LOADERS

### WDF Archive Loader

```cpp
// File: AssetsC3/ArchiveWDF.h
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace Yamen::C3 {

class ArchiveWDF {
public:
    struct Header {
        uint32_t signature;
        uint32_t fileCount;
        uint32_t indexOffset;
    };
    
    struct IndexEntry {
        uint32_t uid;      // Hash ID
        uint32_t size;     // File size
        uint32_t offset;   // File offset
        
        bool operator<(const IndexEntry& other) const {
            return uid < other.uid;
        }
    };
    
    ArchiveWDF() = default;
    ~ArchiveWDF();
    
    bool Open(const char* filename);
    void Close();
    
    void* Load(uint32_t fileID, uint32_t& size);
    bool IsOpen() const { return m_file != nullptr; }
    uint32_t GetArchiveID() const { return m_archiveID; }
    
private:
    FILE* m_file = nullptr;
    std::vector<IndexEntry> m_index;
    uint32_t m_archiveID = 0;
    std::string m_filename;
    
    // Binary search in sorted index
    const IndexEntry* FindEntry(uint32_t fileID) const;
};

} // namespace Yamen::C3
```

```cpp
// File: AssetsC3/ArchiveWDF.cpp
#include "ArchiveWDF.h"
#include "C3Hash.h"
#include <algorithm>
#include <cstring>

namespace Yamen::C3 {

ArchiveWDF::~ArchiveWDF() {
    Close();
}

bool ArchiveWDF::Open(const char* filename) {
    Close();
    
    m_file = fopen(filename, "rb");
    if (!m_file) {
        return false;
    }
    
    m_filename = filename;
    
    // Read header
    Header header;
    if (fread(&header, sizeof(Header), 1, m_file) != 1) {
        Close();
        return false;
    }
    
    // Allocate index
    m_index.resize(header.fileCount);
    
    // Seek to index
    if (fseek(m_file, header.indexOffset, SEEK_SET) != 0) {
        Close();
        return false;
    }
    
    // Read index
    if (fread(m_index.data(), sizeof(IndexEntry), header.fileCount, m_file) != header.fileCount) {
        Close();
        return false;
    }
    
    // Verify index is sorted (should be from WDF creation)
    if (!std::is_sorted(m_index.begin(), m_index.end())) {
        // Sort if needed (shouldn't happen with proper WDF files)
        std::sort(m_index.begin(), m_index.end());
    }
    
    // Compute archive ID
    m_archiveID = HashSystem::StringToID(filename);
    
    return true;
}

void ArchiveWDF::Close() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    m_index.clear();
    m_archiveID = 0;
}

void* ArchiveWDF::Load(uint32_t fileID, uint32_t& size) {
    if (!m_file) {
        size = 0;
        return nullptr;
    }
    
    // Binary search
    const IndexEntry* entry = FindEntry(fileID);
    if (!entry) {
        size = 0;
        return nullptr;
    }
    
    // Allocate buffer
    void* buffer = malloc(entry->size);
    if (!buffer) {
        size = 0;
        return nullptr;
    }
    
    // Seek and read
    if (fseek(m_file, entry->offset, SEEK_SET) != 0) {
        free(buffer);
        size = 0;
        return nullptr;
    }
    
    if (fread(buffer, entry->size, 1, m_file) != 1) {
        free(buffer);
        size = 0;
        return nullptr;
    }
    
    size = entry->size;
    return buffer;
}

const ArchiveWDF::IndexEntry* ArchiveWDF::FindEntry(uint32_t fileID) const {
    // Binary search in sorted array
    auto it = std::lower_bound(
        m_index.begin(), 
        m_index.end(), 
        IndexEntry{fileID, 0, 0}
    );
    
    if (it != m_index.end() && it->uid == fileID) {
        return &(*it);
    }
    
    return nullptr;
}

} // namespace Yamen::C3
```

### DNP Archive Loader

```cpp
// File: AssetsC3/ArchiveDNP.h
#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>
#include <mutex>

namespace Yamen::C3 {

class ArchiveDNP {
public:
    static constexpr char SIGNATURE[] = "DawnPack.TqDigital";
    static constexpr uint32_t VERSION = 1000;
    static constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB
    
    struct FileInfo {
        uint32_t offset;
        uint32_t size;
    };
    
    ArchiveDNP();
    ~ArchiveDNP();
    
    bool Open(const char* filename);
    void Close();
    
    void* Load(uint32_t fileID, uint32_t& size);
    bool IsOpen() const { return m_file != nullptr; }
    
private:
    FILE* m_file = nullptr;
    std::unordered_map<uint32_t, FileInfo> m_index;
    std::unique_ptr<uint8_t[]> m_buffer;
    std::unique_ptr<uint8_t[]> m_extendBuffer;
    std::mutex m_mutex; // Thread safety
};

} // namespace Yamen::C3
```

```cpp
// File: AssetsC3/ArchiveDNP.cpp
#include "ArchiveDNP.h"
#include "C3Hash.h"
#include <cstring>

namespace Yamen::C3 {

ArchiveDNP::ArchiveDNP() {
    m_buffer = std::make_unique<uint8_t[]>(BUFFER_SIZE);
}

ArchiveDNP::~ArchiveDNP() {
    Close();
}

bool ArchiveDNP::Open(const char* filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Close();
    
    m_file = fopen(filename, "rb");
    if (!m_file) {
        return false;
    }
    
    // Read and verify signature
    char signature[32] = {0};
    if (fread(signature, 1, 32, m_file) != 32) {
        Close();
        return false;
    }
    
    if (strcmp(signature, SIGNATURE) != 0) {
        Close();
        return false;
    }
    
    // Read version
    uint32_t version = 0;
    if (fread(&version, sizeof(uint32_t), 1, m_file) != 1) {
        Close();
        return false;
    }
    
    if (version != VERSION) {
        Close();
        return false;
    }
    
    // Read file count
    uint32_t fileCount = 0;
    if (fread(&fileCount, sizeof(uint32_t), 1, m_file) != 1) {
        Close();
        return false;
    }
    
    // Read index entries
    m_index.reserve(fileCount);
    
    for (uint32_t i = 0; i < fileCount; i++) {
        uint32_t fileID = 0;
        uint32_t size = 0;
        uint32_t offset = 0;
        
        if (fread(&fileID, sizeof(uint32_t), 1, m_file) != 1 ||
            fread(&size, sizeof(uint32_t), 1, m_file) != 1 ||
            fread(&offset, sizeof(uint32_t), 1, m_file) != 1) {
            Close();
            return false;
        }
        
        m_index[fileID] = {offset, size};
    }
    
    return true;
}

void ArchiveDNP::Close() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    m_index.clear();
    m_extendBuffer.reset();
}

void* ArchiveDNP::Load(uint32_t fileID, uint32_t& size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_file) {
        size = 0;
        return nullptr;
    }
    
    // Find in hash map
    auto it = m_index.find(fileID);
    if (it == m_index.end()) {
        size = 0;
        return nullptr;
    }
    
    const FileInfo& info = it->second;
    
    // Seek to file data
    if (fseek(m_file, info.offset, SEEK_SET) != 0) {
        size = 0;
        return nullptr;
    }
    
    // Use extended buffer for large files
    void* buffer = nullptr;
    if (info.size > BUFFER_SIZE) {
        m_extendBuffer = std::make_unique<uint8_t[]>(info.size);
        buffer = m_extendBuffer.get();
    } else {
        buffer = m_buffer.get();
    }
    
    // Read file data
    if (fread(buffer, 1, info.size, m_file) != info.size) {
        size = 0;
        return nullptr;
    }
    
    size = info.size;
    return buffer;
}

} // namespace Yamen::C3
```

---

## 3. ASSET MANAGER

### Unified Asset Loading System

```cpp
// File: AssetsC3/AssetManager.h
#pragma once
#include "ArchiveWDF.h"
#include "ArchiveDNP.h"
#include "C3Hash.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace Yamen::C3 {

class Asset {
public:
    virtual ~Asset() = default;
    virtual bool LoadFromMemory(const void* data, uint32_t size) = 0;
    virtual void Unload() = 0;
};

class AssetManager {
public:
    static constexpr int MAX_ARCHIVES = 32;
    
    AssetManager() = default;
    ~AssetManager();
    
    // Archive management
    bool OpenWDF(const char* filename);
    bool OpenDNP(const char* filename);
    void CloseAll();
    
    // Asset loading
    void* LoadRaw(const char* filename, uint32_t& size);
    
    template<typename T>
    std::shared_ptr<T> Load(const char* filename);
    
    // Cache management
    void ClearCache();
    size_t GetCacheSize() const { return m_cache.size(); }
    
private:
    std::vector<std::unique_ptr<ArchiveWDF>> m_wdfArchives;
    std::vector<std::unique_ptr<ArchiveDNP>> m_dnpArchives;
    std::unordered_map<uint32_t, std::shared_ptr<Asset>> m_cache;
    
    void* LoadFromArchives(uint32_t packID, uint32_t fileID, uint32_t& size);
    void* LoadFromFilesystem(const char* filename, uint32_t& size);
};

} // namespace Yamen::C3
```

```cpp
// File: AssetsC3/AssetManager.cpp
#include "AssetManager.h"
#include <fstream>

namespace Yamen::C3 {

AssetManager::~AssetManager() {
    CloseAll();
}

bool AssetManager::OpenWDF(const char* filename) {
    auto archive = std::make_unique<ArchiveWDF>();
    if (!archive->Open(filename)) {
        return false;
    }
    
    m_wdfArchives.push_back(std::move(archive));
    return true;
}

bool AssetManager::OpenDNP(const char* filename) {
    auto archive = std::make_unique<ArchiveDNP>();
    if (!archive->Open(filename)) {
        return false;
    }
    
    m_dnpArchives.push_back(std::move(archive));
    return true;
}

void AssetManager::CloseAll() {
    m_wdfArchives.clear();
    m_dnpArchives.clear();
    m_cache.clear();
}

void* AssetManager::LoadRaw(const char* filename, uint32_t& size) {
    if (!filename) {
        size = 0;
        return nullptr;
    }
    
    // Generate hash IDs
    uint32_t packID = HashSystem::PackName(filename);
    uint32_t fileID = HashSystem::RealName(filename);
    
    // Try archives first
    void* data = LoadFromArchives(packID, fileID, size);
    if (data) {
        return data;
    }
    
    // Fallback to filesystem
    return LoadFromFilesystem(filename, size);
}

void* AssetManager::LoadFromArchives(uint32_t packID, uint32_t fileID, uint32_t& size) {
    // Try WDF archives
    for (auto& archive : m_wdfArchives) {
        if (archive->GetArchiveID() == packID) {
            void* data = archive->Load(fileID, size);
            if (data) {
                return data;
            }
        }
    }
    
    // Try DNP archives
    for (auto& archive : m_dnpArchives) {
        void* data = archive->Load(fileID, size);
        if (data) {
            return data;
        }
    }
    
    size = 0;
    return nullptr;
}

void* AssetManager::LoadFromFilesystem(const char* filename, uint32_t& size) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        size = 0;
        return nullptr;
    }
    
    size = static_cast<uint32_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    void* buffer = malloc(size);
    if (!buffer) {
        size = 0;
        return nullptr;
    }
    
    if (!file.read(static_cast<char*>(buffer), size)) {
        free(buffer);
        size = 0;
        return nullptr;
    }
    
    return buffer;
}

template<typename T>
std::shared_ptr<T> AssetManager::Load(const char* filename) {
    uint32_t fileID = HashSystem::RealName(filename);
    
    // Check cache
    auto it = m_cache.find(fileID);
    if (it != m_cache.end()) {
        return std::static_pointer_cast<T>(it->second);
    }
    
    // Load raw data
    uint32_t size = 0;
    void* data = LoadRaw(filename, size);
    if (!data) {
        return nullptr;
    }
    
    // Create asset
    auto asset = std::make_shared<T>();
    if (!asset->LoadFromMemory(data, size)) {
        free(data);
        return nullptr;
    }
    
    free(data);
    
    // Cache it
    m_cache[fileID] = asset;
    
    return asset;
}

void AssetManager::ClearCache() {
    m_cache.clear();
}

} // namespace Yamen::C3
```

---

## 4. EXAMPLE USAGE

```cpp
// File: Examples/LoadingExample.cpp
#include "AssetManager.h"
#include "ModelPHY.h"
#include "AnimationANI.h"
#include "TextureDDS.h"
#include <iostream>

using namespace Yamen::C3;

int main() {
    // Initialize asset system
    AssetManager assets;
    
    // Open game archives
    if (!assets.OpenWDF("data/models.wdf")) {
        std::cerr << "Failed to open models.wdf" << std::endl;
        return 1;
    }
    
    if (!assets.OpenWDF("data/textures.wdf")) {
        std::cerr << "Failed to open textures.wdf" << std::endl;
        return 1;
    }
    
    if (!assets.OpenDNP("data/animations.dnp")) {
        std::cerr << "Failed to open animations.dnp" << std::endl;
        return 1;
    }
    
    // Load warrior model
    auto model = assets.Load<ModelPHY>("data/models/warrior.phy");
    if (!model) {
        std::cerr << "Failed to load warrior model" << std::endl;
        return 1;
    }
    
    std::cout << "Model loaded: " << model->GetBoneCount() << " bones, "
              << model->GetMeshCount() << " meshes" << std::endl;
    
    // Load texture
    auto texture = assets.Load<TextureDDS>("texture/warrior_body.dds");
    if (!texture) {
        std::cerr << "Failed to load texture" << std::endl;
        return 1;
    }
    
    std::cout << "Texture loaded: " << texture->GetWidth() << "x" 
              << texture->GetHeight() << std::endl;
    
    // Load animation
    auto anim = assets.Load<AnimationANI>("data/animations/warrior_walk.ini");
    if (!anim) {
        std::cerr << "Failed to load animation" << std::endl;
        return 1;
    }
    
    std::cout << "Animation loaded: " << anim->GetFrameCount() 
              << " frames" << std::endl;
    
    // Game loop simulation
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    for (int frame = 0; frame < 300; frame++) { // 5 seconds
        model->Update(deltaTime);
        model->SetTexture(texture);
        model->PlayAnimation(anim);
        
        // Render would happen here
        // renderer->DrawModel(model, camera);
    }
    
    // Cleanup (automatic with smart pointers)
    std::cout << "Cache size: " << assets.GetCacheSize() << " assets" << std::endl;
    assets.ClearCache();
    
    return 0;
}
```

---

## 5. TESTING & VALIDATION

### Hash Algorithm Test

```cpp
// File: Tests/HashTest.cpp
#include "C3Hash.h"
#include <cassert>
#include <iostream>

using namespace Yamen::C3;

void TestHashAlgorithm() {
    // Known test vectors (you need to generate these from original client)
    struct TestCase {
        const char* input;
        uint32_t expectedHash;
    };
    
    TestCase tests[] = {
        {"data/models", 0x12345678},  // Replace with actual hash
        {"texture/warrior_body.dds", 0xABCDEF00},  // Replace
        // Add more test cases
    };
    
    for (const auto& test : tests) {
        uint32_t hash = HashSystem::StringToID(test.input);
        std::cout << "Input: " << test.input << std::endl;
        std::cout << "Expected: 0x" << std::hex << test.expectedHash << std::endl;
        std::cout << "Got: 0x" << std::hex << hash << std::endl;
        
        assert(hash == test.expectedHash);
    }
    
    std::cout << "All hash tests passed!" << std::endl;
}

int main() {
    TestHashAlgorithm();
    return 0;
}
```

### Archive Loading Test

```cpp
// File: Tests/ArchiveTest.cpp
#include "ArchiveWDF.h"
#include "C3Hash.h"
#include <iostream>

using namespace Yamen::C3;

void TestWDFLoading() {
    ArchiveWDF archive;
    
    if (!archive.Open("test_data/test.wdf")) {
        std::cerr << "Failed to open test archive" << std::endl;
        return;
    }
    
    std::cout << "Archive opened successfully" << std::endl;
    
    // Try to load a known file
    uint32_t fileID = HashSystem::RealName("test_file.dat");
    uint32_t size = 0;
    void* data = archive.Load(fileID, size);
    
    if (data) {
        std::cout << "File loaded: " << size << " bytes" << std::endl;
        
        // Verify content if known
        // ...
        
        free(data);
    } else {
        std::cerr << "File not found in archive" << std::endl;
    }
    
    archive.Close();
}

int main() {
    TestWDFLoading();
    return 0;
}
```

---

## 6. INTEGRATION WITH YAMEN ENGINE

### Directory Structure

```
Yamen/
├── Core/
│   ├── C3Hash.h
│   └── C3Hash.cpp
├── AssetsC3/
│   ├── ArchiveWDF.h
│   ├── ArchiveWDF.cpp
│   ├── ArchiveDNP.h
│   ├── ArchiveDNP.cpp
│   ├── AssetManager.h
│   ├── AssetManager.cpp
│   ├── ModelPHY.h
│   ├── ModelPHY.cpp
│   ├── AnimationANI.h
│   ├── AnimationANI.cpp
│   ├── TextureDDS.h
│   └── TextureDDS.cpp
├── Tests/
│   ├── HashTest.cpp
│   ├── ArchiveTest.cpp
│   └── AssetLoadTest.cpp
└── Examples/
    └── LoadingExample.cpp
```

### Premake5 Configuration

```lua
-- File: premake5.lua
workspace "Yamen"
    architecture "x64"
    configurations { "Debug", "Release" }
    
project "YamenCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    
    files {
        "Core/**.h",
        "Core/**.cpp"
    }
    
project "YamenAssetsC3"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    
    files {
        "AssetsC3/**.h",
        "AssetsC3/**.cpp"
    }
    
    links {
        "YamenCore"
    }
    
project "YamenTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    
    files {
        "Tests/**.cpp"
    }
    
    links {
        "YamenCore",
        "YamenAssetsC3"
    }
```

---

## 7. PERFORMANCE OPTIMIZATIONS

### Memory Pool for Assets

```cpp
template<typename T, size_t POOL_SIZE = 1024>
class AssetPool {
public:
    std::shared_ptr<T> Allocate() {
        if (m_freeList.empty()) {
            return std::make_shared<T>();
        }
        
        auto asset = m_freeList.back();
        m_freeList.pop_back();
        return asset;
    }
    
    void Deallocate(std::shared_ptr<T> asset) {
        asset->Reset();
        m_freeList.push_back(asset);
    }
    
private:
    std::vector<std::shared_ptr<T>> m_freeList;
};
```

### Async Loading

```cpp
class AsyncLoader {
public:
    template<typename T>
    std::future<std::shared_ptr<T>> LoadAsync(const char* filename) {
        return std::async(std::launch::async, [this, filename]() {
            return m_assetManager.Load<T>(filename);
        });
    }
    
private:
    AssetManager& m_assetManager;
};
```

---

## 8. CRITICAL NOTES

### Endianness
The original code assumes little-endian (x86). If targeting big-endian platforms:
- Add byte swapping for all binary reads
- Test thoroughly on target platform

### Thread Safety
Original code has minimal thread safety. Added:
- Mutex in DNP loader
- Consider reader-writer locks for better performance
- Separate asset manager per thread for parallel loading

### Memory Management
- Use smart pointers (std::shared_ptr, std::unique_ptr)
- RAII for automatic cleanup
- Pool allocators for frequent allocations

---

**END OF IMPLEMENTATION GUIDE**

This code provides a solid foundation for the Yamen Engine C3 asset system.
Test thoroughly with original Conquer Online 2.0 archives before extending.
