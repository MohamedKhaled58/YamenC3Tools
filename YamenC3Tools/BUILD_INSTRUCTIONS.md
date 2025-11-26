# Yamen C3 Tools - Build Instructions

## Prerequisites

1. **Windows 10/11**
2. **Visual Studio 2022** (Community/Professional/Enterprise)
   - Install "Desktop development with C++" workload
   - Include "Windows 10/11 SDK"
3. **Premake5** 
   - Download from: https://premake.github.io/download
   - Add to PATH or place in project root

## Step 1: Clone Repository
```bash
git clone --recursive https://github.com/yourusername/YamenC3Tools.git
cd YamenC3Tools
```

**Important:** Use `--recursive` to clone submodules (ImGui, JSON, etc.)

If you forgot `--recursive`, run:
```bash
git submodule update --init --recursive
```

## Step 2: Generate Visual Studio Solution
```bash
premake5 vs2022
```

This creates `YamenC3Tools.sln`

## Step 3: Build

**Option A - Visual Studio:**
1. Open `YamenC3Tools.sln`
2. Select Configuration (Debug/Release)
3. Build → Build Solution (Ctrl+Shift+B)

**Option B - Command Line:**
```bash
msbuild YamenC3Tools.sln /p:Configuration=Release /p:Platform=x64
```

## Step 4: Run

Executable location:
```
bin/Release-windows-x86_64/YamenC3Tools/YamenC3Tools.exe
```

## Troubleshooting

### Error: "Cannot open include file 'imgui.h'"
**Solution:** Submodules not initialized
```bash
git submodule update --init --recursive
```

### Error: "d3d11.lib not found"
**Solution:** Install Windows SDK via Visual Studio Installer

### Error: "Premake not found"
**Solution:** Download premake5.exe and add to PATH

### Linker Error: LNK2019
**Solution:** Rebuild entire solution (Clean → Build)

## Directory Structure After Build
```
YamenC3Tools/
├── bin/
│   └── Release-windows-x86_64/
│       └── YamenC3Tools/
│           └── YamenC3Tools.exe
├── bin-int/          (intermediate build files)
├── third_party/      (git submodules)
├── src/              (source code)
└── assets/           (shaders)
```

## Clean Build
```bash
# Delete generated files
rmdir /s /q bin
rmdir /s /q bin-int
del /q *.sln
del /q *.vcxproj*

# Regenerate
premake5 vs2022
```

## Release Package

To create a distributable package:

1. Build in Release mode
2. Copy from `bin/Release-windows-x86_64/YamenC3Tools/`:
   - YamenC3Tools.exe
   - Any .dll files (if present)
3. Include `assets/` folder if needed
4. Create ZIP

## Support

- GitHub Issues: https://github.com/yourusername/YamenC3Tools/issues
- Documentation: See README.md