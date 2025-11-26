# Yamen C3 Tools - Testing Checklist

## 🧪 Pre-Release Testing

### ✅ Build Tests

- [ ] Clean build succeeds (Debug)
- [ ] Clean build succeeds (Release)
- [ ] No compiler warnings
- [ ] All submodules initialized correctly
- [ ] Premake generates solution without errors

### ✅ Core Functionality

#### File Loading
- [ ] PHY files load correctly
- [ ] PHY3 files load correctly
- [ ] PHY4 files load correctly
- [ ] SMOT files load correctly
- [ ] PTCL files load correctly
- [ ] Invalid files show error message
- [ ] Large files (>10MB) load without crash
- [ ] Multiple files can be loaded in sequence

#### Rendering
- [ ] Model renders in viewport
- [ ] Camera rotation works
- [ ] Camera zoom works
- [ ] Camera pan works
- [ ] Wireframe mode toggles correctly
- [ ] Lighting is visible
- [ ] No black screen on startup
- [ ] Resizing window updates viewport

#### Morph Targets
- [ ] Slider adjustment blends morphs
- [ ] Reset button restores defaults
- [ ] Animation plays smoothly
- [ ] Animation speed control works
- [ ] Weights sum to 1.0 (or don't break)

#### Export
- [ ] glTF export creates files
- [ ] glTF files open in Blender
- [ ] Morph targets preserved in export
- [ ] Vertex colors preserved
- [ ] UV coordinates correct
- [ ] Indices/topology intact

### ✅ UI Tests

#### ImGui Interface
- [ ] Main menu bar visible
- [ ] File dialogs open correctly
- [ ] Properties panel displays info
- [ ] Status bar updates
- [ ] No UI overlap issues
- [ ] Docking works (if enabled)
- [ ] Viewports work (if enabled)

#### Input Handling
- [ ] Mouse not blocked by UI when over viewport
- [ ] Keyboard shortcuts work
- [ ] ESC key exits application
- [ ] UI captures input when over panels

### ✅ Error Handling

- [ ] Missing file shows error, doesn't crash
- [ ] Corrupted C3 file shows error
- [ ] Export to read-only folder shows error
- [ ] Out of memory handled gracefully
- [ ] DirectX device loss handled

### ✅ Performance

- [ ] 60 FPS with small models (<5K verts)
- [ ] 30+ FPS with large models (>50K verts)
- [ ] No memory leaks (run for 10 minutes)
- [ ] Smooth animation playback
- [ ] UI remains responsive during operations

### ✅ Platform Specific

#### Windows 10
- [ ] Application launches
- [ ] No missing DLL errors
- [ ] File associations work (future)

#### Windows 11
- [ ] Application launches
- [ ] Dark mode compatible
- [ ] High DPI scaling correct

### ✅ Edge Cases

- [ ] Load file with 0 vertices (should error)
- [ ] Load file with 1 million vertices (stress test)
- [ ] Rapid file loading doesn't crash
- [ ] Spam morph sliders doesn't crash
- [ ] Minimize/restore window works
- [ ] Alt+Tab doesn't cause issues

### ✅ Documentation

- [ ] README.md is accurate
- [ ] BUILD_INSTRUCTIONS.md works
- [ ] QUICK_START.md tested by new user
- [ ] All example workflows tested
- [ ] Code comments are helpful

---

## 🐛 Known Issues (Track These)

### Critical (Must Fix Before Release)
- [ ] None currently

### Major (Fix ASAP)
- [ ] glTF import not implemented
- [ ] OBJ export not implemented
- [ ] Batch processing not implemented

### Minor (Fix Eventually)
- [ ] No texture loading yet
- [ ] No screenshot feature
- [ ] No mesh optimization

### Enhancement (Future)
- [ ] FBX support
- [ ] Animation timeline
- [ ] Skeletal animation playback
- [ ] Material editor

---

## 📊 Test Results Template
```
Test Date: YYYY-MM-DD
Tester: [Name]
Build: [Debug/Release]
OS: [Windows 10/11]

Test Summary:
- Total Tests: 50
- Passed: 47
- Failed: 3
- Blocked: 0

Failed Tests:
1. Test Name: Export to glTF
   Expected: Creates .gltf and .bin
   Actual: Only .gltf created, .bin empty
   Severity: Major
   Issue #: 42

2. Test Name: Large file loading
   Expected: Loads without crash
   Actual: Out of memory error
   Severity: Minor
   Issue #: 43

3. Test Name: Camera reset hotkey
   Expected: Pressing R resets camera
   Actual: Not implemented
   Severity: Enhancement
   Issue #: 44
```

---

## 🚀 Release Criteria

Before marking as v1.0:
- [ ] ✅ All Critical tests pass
- [ ] ✅ All Major tests pass (or issues documented)
- [ ] ✅ Core features work (load, view, export)
- [ ] ✅ Documentation complete
- [ ] ✅ No known crashes
- [ ] ✅ Tested by 3+ users

---

## 👥 Testing Assignments

- **Build Tests:** Developer
- **Core Functionality:** QA + Developer
- **UI Tests:** UX Designer
- **Performance:** QA
- **Documentation:** Technical Writer
- **User Acceptance:** Beta Users

---

**Update this checklist as you test!**