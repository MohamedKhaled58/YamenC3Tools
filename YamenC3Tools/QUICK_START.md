# Yamen C3 Tools - Quick Start Guide

## 🚀 Getting Started in 5 Minutes

### 1. Build the Project
```bash
# Clone with submodules
git clone --recursive https://github.com/yourusername/YamenC3Tools.git
cd YamenC3Tools

# Generate Visual Studio solution
premake5 vs2022

# Build (or open .sln in Visual Studio)
msbuild YamenC3Tools.sln /p:Configuration=Release /p:Platform=x64
```

### 2. Launch the Application
```bash
cd bin/Release-windows-x86_64/YamenC3Tools
YamenC3Tools.exe
```

### 3. Load Your First C3 File

1. **File → Open C3...** (or Ctrl+O)
2. Select a `.c3` file from your Conquer Online installation
3. The model appears in the 3D viewport

### 4. Navigate the 3D View

| Control | Action |
|---------|--------|
| **Left Mouse Drag** | Rotate camera around model |
| **Mouse Wheel** | Zoom in/out |
| **Middle Mouse Drag** | Pan camera |
| **Right Click** | Context menu (future) |

### 5. Adjust Morph Targets

In the **Properties** panel (left side):
- Slide **Morph Target** sliders to blend between shapes
- Click **Play** in Animation section to auto-animate
- Adjust **Speed** slider to control animation speed

### 6. Export to Blender

1. **File → Export to glTF...**
2. Choose save location
3. In Blender: **File → Import → glTF 2.0**
4. Your C3 model is now in Blender with morph targets!

---

## 📚 Common Workflows

### Workflow 1: View Character Model
```
1. Open warrior.c3
2. Rotate with mouse to inspect
3. Enable wireframe: View → Wireframe
4. Take screenshot
```

### Workflow 2: Export for External Use
```
1. Open model.c3
2. File → Export to glTF
3. Import in Blender/Unity/Unreal
4. Apply materials and textures
```

### Workflow 3: Animate Morph Targets
```
1. Open face.c3
2. Properties → Morph Targets
3. Manually adjust sliders for expressions
4. Or click Play for auto-animation
5. Record screen for preview
```

### Workflow 4: Batch Convert Files
```
1. File → Batch Process (future feature)
2. Select input folder
3. Choose output format
4. Convert all at once
```

---

## 🎨 UI Overview
```
┌─────────────────────────────────────────────────────────┐
│ File  View  Help                                        │ Menu Bar
├──────────┬──────────────────────────────────────────────┤
│          │                                              │
│ Model    │          3D VIEWPORT                         │
│ Info     │      (Your C3 model renders here)            │
│          │                                              │
│ Morph    │     Use mouse to rotate/zoom/pan             │
│ Targets  │                                              │
│          │                                              │
│ Animation│                                              │
│ Controls │                                              │
│          │                                              │
│ Camera   │                                              │
│ Settings │                                              │
│          │                                              │
├──────────┴──────────────────────────────────────────────┤
│ Status: Ready. Load a C3 file to begin.                 │ Status Bar
└─────────────────────────────────────────────────────────┘
```

---

## 🔍 Testing Your Installation

### Test 1: Basic Load
```
1. Launch app
2. Open any .c3 file
3. Verify model appears
✅ SUCCESS: You see a 3D model
❌ FAIL: Error message or crash → Check BUILD_INSTRUCTIONS.md
```

### Test 2: Camera Controls
```
1. Drag left mouse → Model rotates
2. Scroll wheel → Model zooms
3. Drag middle mouse → Camera pans
✅ SUCCESS: All controls work smoothly
❌ FAIL: Controls don't respond → Check if ImGui is capturing input
```

### Test 3: Export
```
1. Load a model
2. File → Export to glTF
3. Check output folder for .gltf and .bin
✅ SUCCESS: Both files created
❌ FAIL: Error message → Check write permissions
```

---

## ⚙️ Configuration

### Default Settings
- **Window Size:** 1600x900
- **Camera FOV:** 45°
- **Animation Speed:** 1.0x
- **Morph Weights:** [1.0, 0.0, 0.0, 0.0]

### Customization
Edit `src/Main.cpp` and modify `AppState` struct:
```cpp
int windowWidth = 1920;  // Your resolution
int windowHeight = 1080;
float animationSpeed = 2.0f;  // Faster animation
```

Rebuild after changes.

---

## 🐛 Troubleshooting

### Problem: "Failed to create window"
**Solution:** Update graphics drivers

### Problem: "Failed to initialize renderer"
**Solution:** Check DirectX 11 support
```bash
dxdiag  # Run this and check "Display" tab for "Feature Levels"
```

### Problem: Model loads but appears black
**Solution:** Lighting issue - toggle wireframe to verify geometry

### Problem: Export creates empty files
**Solution:** Model might have no geometry - check Model Info panel

### Problem: Animation doesn't play
**Solution:** Model might not have morph targets - only works with PHY4

---

## 📖 Next Steps

1. **Read** `README.md` for full feature list
2. **Check** `BUILD_INSTRUCTIONS.md` for advanced build options
3. **Explore** Example C3 files in `examples/` folder (if provided)
4. **Contribute** by opening issues or pull requests on GitHub

---

## 💡 Pro Tips

1. **Hold Shift** while dragging = faster camera movement (future)
2. **Double-click** model = focus camera on center
3. **Press R** = reset camera to default view
4. **Press Space** = play/pause animation
5. **Press F** = toggle fullscreen (future)

---

## 🎓 Learn More

- **C3 Format Documentation:** See `C3_File_Format_Complete_Analysis.md`
- **Video Tutorials:** [Coming soon]
- **Community Discord:** [Link here]
- **Report Bugs:** GitHub Issues

---

**Ready to master C3 assets? Start experimenting!** 🎮