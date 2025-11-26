# Yamen C3 Tools - Usage Examples

## Example 1: Character Model Inspection

**Goal:** View a warrior character model from all angles
```
Steps:
1. Launch YamenC3Tools.exe
2. File → Open C3... → Select "warrior.c3"
3. Left-drag mouse to rotate 360°
4. Mouse wheel to zoom close on face details
5. View → Wireframe to see polygon structure
6. Properties → Model Info to see vertex count

Expected Output:
- Model rotates smoothly
- Wireframe shows triangle mesh
- Info panel displays: "Type: PHY4, Meshes: 8, Vertices: 2,450"
```

---

## Example 2: Facial Expression Animation

**Goal:** Animate character facial morph targets
```
Steps:
1. Open "character_head.c3"
2. Properties Panel → Morph Targets section
3. Adjust sliders manually:
   - Target 0 (Base): 0.5
   - Target 1 (Smile): 0.5
   - Target 2 (Frown): 0.0
   - Target 3 (Surprised): 0.0
4. Observe face blend between base and smile
5. Click "Play" button in Animation section
6. Watch automatic morph animation cycle

Expected Output:
- Face smoothly transitions between expressions
- Animation loops continuously
- Can adjust speed slider (0.1x to 5.0x)
```

---

## Example 3: Export to Blender

**Goal:** Convert C3 model to glTF for Blender editing
```
Steps:
1. Open "armor_set.c3"
2. Verify model looks correct in viewport
3. File → Export to glTF...
4. Save as: "armor_set" (auto adds .gltf extension)
5. Close YamenC3Tools
6. Open Blender 4.0+
7. File → Import → glTF 2.0 (.gltf/.glb)
8. Select "armor_set.gltf"
9. In Blender, check:
   - Object appears in scene
   - Geometry Data → Shape Keys shows morph targets
   - Vertex colors preserved
   - UV maps intact

Expected Output:
- armor_set.gltf (JSON file)
- armor_set.bin (binary data)
- Blender successfully imports with all data
```

---

## Example 4: Weapon Trail Visualization

**Goal:** View SMOT weapon trail effect
```
Steps:
1. Open "sword_trail.c3" (Type: SMOT)
2. Properties → Model Info shows:
   - Type: SMOT
   - Shapes: 1
   - Lines: 3
3. Viewport shows trail path geometry
4. Animation → Play to see trail animation
5. View → Wireframe to see line structure

Expected Output:
- Trail path visible as connected line segments
- Animates along weapon swing path
- Smooth curve interpolation
```

---

## Example 5: Particle System Preview

**Goal:** Visualize PTCL particle emitter
```
Steps:
1. Open "fire_effect.c3" (Type: PTCL)
2. Properties → Model Info shows:
   - Type: PTCL
   - Particles: 1 system
   - Max Particles: 1000
3. Animation → Play
4. Observe particles emitting from origin
5. Properties → Particle Settings:
   - Adjust emit rate (0-100)
   - Modify lifetime (0-10 seconds)
   - Change start/end colors

Expected Output:
- Particles spawn continuously
- Fade out over lifetime
- Billboard sprites always face camera
```

---

## Example 6: Batch Export Multiple Models

**Goal:** Convert entire folder of C3 files to glTF
```
Steps (Future Feature):
1. File → Batch Process
2. Source Folder: "C:\ConquerOnline\models\"
3. Output Folder: "C:\Exports\"
4. Format: glTF 2.0
5. Options:
   ☑ Export Morph Targets
   ☑ Export Vertex Colors
   ☐ Export Animations (if available)
6. Click "Start Batch"
7. Progress bar shows: "Processing 45/120 files..."
8. Completion: "120 files converted successfully"

Expected Output:
- 120 .gltf files in output folder
- 120 .bin files
- Log file: batch_export.log
```

---

## Example 7: Compare Two Versions

**Goal:** Visual diff between old and new model
```
Steps (Future Feature):
1. File → Compare Models
2. Load Model A: "warrior_v1.c3"
3. Load Model B: "warrior_v2.c3"
4. View modes:
   - Side-by-side
   - Overlay with transparency
   - Difference heatmap (vertex displacement)
5. Stats comparison table:
   | Property | V1 | V2 | Delta |
   |----------|----|----|-------|
   | Vertices | 2450 | 3120 | +670 |
   | Triangles | 4200 | 5800 | +1600 |

Expected Output:
- Visual differences highlighted
- Geometric changes quantified
```

---

## Example 8: Screenshot for Documentation

**Goal:** Capture high-quality model renders
```
Steps (Future Feature):
1. Load "boss_monster.c3"
2. Position camera for best angle
3. View → Rendering:
   ☑ Smooth Shading
   ☑ Ambient Occlusion
   ☑ Shadows
   ☐ Wireframe
4. Tools → Screenshot
5. Options:
   - Resolution: 4K (3840x2160)
   - Background: Transparent PNG
   - Anti-aliasing: 8x MSAA
6. Save as: "boss_monster_render.png"

Expected Output:
- High-res PNG with transparent background
- Suitable for wikis/documentation
```

---

## Example 9: Debug Vertex Attributes

**Goal:** Inspect vertex data for debugging
```
Steps (Future Feature):
1. Load problematic model
2. Tools → Vertex Inspector
3. Click any vertex in viewport
4. Properties show:
   - Position: [1.245, 0.832, -0.456]
   - Normal: [0.577, 0.577, 0.577]
   - UV: [0.342, 0.891]
   - Color: ARGB(255, 200, 180, 160)
   - Bone Indices: [3, 7]
   - Bone Weights: [0.7, 0.3]
5. Highlight all vertices with weight > 0.5 on bone 3

Expected Output:
- Selected vertices highlighted in viewport
- Data table shows all vertex attributes
```

---

## Example 10: Optimize Model for Game Engine

**Goal:** Reduce poly count for mobile
```
Steps (Future Feature):
1. Load "high_poly_character.c3"
2. Tools → Mesh Optimization
3. Target poly count: 50% reduction
4. Options:
   ☑ Preserve UV seams
   ☑ Preserve morph targets
   ☐ Preserve vertex colors
5. Preview optimized mesh
6. Export optimized → "character_mobile.c3"

Expected Output:
- Vertices: 5,000 → 2,500 (50% reduction)
- Visual quality: >95% similar
- File size: 2.3 MB → 1.1 MB
```

---

## 🎯 Validation Checklist

After each example, verify:
- ✅ No crashes or errors
- ✅ Output files created correctly
- ✅ Performance is smooth (>30 FPS)
- ✅ UI remains responsive
- ✅ Memory usage is reasonable (<500 MB)

---

## 📝 Reporting Issues

If an example doesn't work:

1. **Note the exact steps** you took
2. **Capture error message** (screenshot)
3. **Check log file:** `YamenC3Tools.log`
4. **Report on GitHub** with:
   - OS version
   - C3 file type (PHY/SMOT/PTCL)
   - Expected vs actual behavior

---

**Happy C3 asset exploring!** 🎮
```

### FILE 38: `LICENSE`
```
MIT License

Copyright (c) 2024 Yamen C3 Tools Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.