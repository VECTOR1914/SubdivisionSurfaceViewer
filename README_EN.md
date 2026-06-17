# 🎨 Subdivision Surface Viewer — Interactive 3D Mesh Subdivision Explorer

> **One EXE, three classic algorithms, sixteen gorgeous models — watch rough polygons transform into silky-smooth surfaces in real time.**

<p align="center">
  <img src="https://img.shields.io/badge/Language-C++-00599C?style=flat-square&logo=c%2B%2B" alt="C++">
  <img src="https://img.shields.io/badge/Graphics-OpenGL-5587A2?style=flat-square&logo=opengl" alt="OpenGL">
  <img src="https://img.shields.io/badge/UI-GDI+-5C2D91?style=flat-square" alt="GDI+">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/platform-Windows-blue?style=flat-square&logo=windows" alt="Windows">
</p>

<p align="center">
  <img src="screenshots/屏幕截图 2026-06-16 052708.png" alt="Subdivision Surface Viewer Main UI" width="80%">
</p>

---

## ✨ Overview

**Subdivision Surface Viewer** is an interactive Windows desktop application that brings the most elegant algorithms in computer graphics — **Subdivision Surfaces** — to life. Drag your mouse to orbit around 3D models, and watch as coarse low-poly meshes evolve step-by-step into beautifully smooth, high-resolution surfaces.

The project ships as a **single EXE** with **16 preset 3D models** embedded directly in the binary (Stanford Bunny, Utah Teapot, dinosaur, human face, and more). No installer, no internet connection, no external data files — just download and run.

---

## 🧠 Three Algorithms, One Tool

| Algorithm | Type | Input Mesh | Highlights |
|-----------|------|------------|------------|
| **Loop** | Approximating | Triangles | C² continuous at regular vertices; proposed by Charles Loop (1987) |
| **Catmull-Clark** | Approximating | Quads / Polygons | The industry standard — foundational to all Pixar-quality surface modeling |
| **Butterfly** 🆕 | Interpolating | Triangles | 8-point stencil; original vertices preserved exactly (Dyn, Levin & Gregory, 1990) |

> 💡 **A/B comparison at your fingertips** — load a model, press `M` to cycle through all three algorithms, and instantly grasp the difference between approximating and interpolating subdivision schemes.

---

## 🎬 Features

- 🖱️ **Smooth Orbit Camera** — left-drag to rotate, right-drag to pan, scroll to zoom. 60 FPS.
- 🔢 **Subdivision Level 0–4** — press `[` / `]` to step through levels; face count quadruples each level.
- 📦 **16 Built-in Models** — self-contained EXE, models embedded as Windows resources. Zero setup.
- 🎨 **Dark-Themed UI** — custom GDI+ hand-drawn panels with rounded buttons, hover effects, and neon-green accents.
- 📂 **OBJ Import / Export** — load external Wavefront OBJ files, subdivide, and export the result.
- 🧵 **Wireframe ⇄ Solid** — spacebar to toggle between clean wireframe and filled rendering.
- 📐 **Ortho ⇄ Perspective** — press `V` to switch between engineering and cinematic views.
- 📊 **Real-Time HUD** — FPS, active algorithm, current level, vertex/face count always visible.

---

## ⌨️ Controls

### 🖱️ Mouse

| Action | Function |
|--------|----------|
| Left-drag | Orbit / rotate |
| Right-drag | Pan |
| Scroll wheel | Zoom |
| Panel scroll | Scroll the side panel |

### ⌨️ Keyboard

| Key | Function |
|-----|----------|
| `0` – `4` | Jump to subdivision level |
| `[` / `]` | Decrease / increase level |
| `M` | Cycle algorithm (Loop → Catmull-Clark → Butterfly) |
| `Space` | Wireframe ⇄ Solid |
| `V` | Orthographic ⇄ Perspective |
| `O` | Open an external OBJ file |
| `S` | Reset camera |
| `Q` | Quit |

---

## 🔧 Build & Run

### Prerequisites

- **Visual Studio 2019+** with MSVC toolchain
- **Windows SDK** (includes OpenGL & GDI+ headers)
- No extra dependencies — `glm` math library included (header-only)

### Quick Start

```bash
# 1. Open the solution
Double-click ModelViewer.sln

# 2. Select configuration
Debug / Release, x64 / Win32

# 3. Build & run
Press F5
```

### Linked Libraries

| Library | Purpose |
|---------|---------|
| `opengl32.lib` | OpenGL rendering |
| `glu32.lib` | GLU utilities (gluPerspective, etc.) |
| `gdiplus.lib` | Right-side UI panel |
| `comctl32.lib` | Windows common controls |
| `comdlg32.lib` | File open/save dialogs |

---

## 📁 Project Structure

```
.
├── main.cpp                      # 🏠 Win32 entry point — window, message loop, global state
├── Mesh.h / Mesh.cpp             # 📐 Half-edge mesh data structure + OBJ I/O
├── Renderer.h / Renderer.cpp     # 🎨 OpenGL renderer — lighting, colors, wireframe
├── Camera.h / Camera.cpp         # 📷 Arcball orbit camera — rotation, pan, zoom
├── Panel.h / Panel.cpp           # 🎛️  GDI+ control panel — buttons, state display
├── PresetPanel.h / PresetPanel.cpp # 📋 Preset model browser panel
├── SubdivisionBase.h             # 🧬 Abstract base class for subdivision algorithms
├── SubdivisionLoop.h / .cpp      # 🔺 Loop subdivision (approximating · triangles)
├── SubdivisionCatmullClark.h/.cpp # 🔲 Catmull-Clark subdivision (approximating · polygons)
├── SubdivisionButterfly.h / .cpp  # 🦋 Butterfly subdivision (interpolating · triangles)
├── glm/                          # 📦 OpenGL Mathematics (header-only)
├── models/                       # 🗿 16 sample OBJ models
├── scripts/
│   ├── embed_models.py           # 🔨 OBJ → Windows resource embedder
│   └── md2docx.py                # 📝 Markdown → Word converter
└── ModelViewer.sln               # 🏗️  Visual Studio solution file
```

---

## 🌟 Highlights

### 1️⃣ Textbook-Quality Algorithm Implementations

Each subdivision scheme is a clean, self-contained class inheriting from `SubdivisionBase`. Whether you're taking a computer graphics course or curious about the math behind Pixar's animation pipeline, this codebase is an excellent reference.

### 2️⃣ Single EXE, Zero Friction

All 16 models (~27 MB total) are embedded as Windows resources. Ship one file to a friend and they can explore subdivision surfaces immediately — no "download the models folder first" friction.

### 3️⃣ Hand-Crafted Dark UI

No MFC, no Qt — just raw GDI+ pixel-pushing. Rounded buttons, hover highlights, green accent toggles. Minimalist and functional.

### 4️⃣ Rich Preset Library

Stanford Bunny (the "Hello World" of CG), Utah Teapot (the rendering test classic), dinosaur, cat, dog, deer, human face, cow, spaceship, and more — covering a wide range of geometric topologies.

---

## 📸 Screenshots

<p align="center">
  <img src="screenshots/屏幕截图 2026-06-16 053044.png" alt="Screenshot 2" width="45%">
  <img src="screenshots/屏幕截图 2026-06-16 053109.png" alt="Screenshot 3" width="45%">
</p>

<p align="center">
  <img src="screenshots/屏幕截图 2026-06-17 200219.png" alt="Screenshot 4" width="45%">
  <img src="screenshots/屏幕截图 2026-06-17 200242.png" alt="Screenshot 5" width="45%">
</p>

<p align="center">
  <img src="screenshots/屏幕截图 2026-06-17 200335.png" alt="Screenshot 6" width="45%">
  <img src="screenshots/屏幕截图 2026-06-17 200408.png" alt="Screenshot 7" width="45%">
</p>

---

## 🧪 Technical Details

### Loop Subdivision

- New edge vertex: `3/8(v₀+v₁) + 1/8(v₂+v₃)`
- Original vertex update: `(1 - nβ)v + β·Σ neighbors`
- Weight: `β = 1/n · (5/8 - (3/8 + 1/4·cos(2π/n))²)`
- C² continuous at regular vertices

### Catmull-Clark Subdivision

- Face point = average of face vertices
- Edge point = (edge endpoints + adjacent face points) / 2
- New vertex = `(F + 2R + (n-3)P) / n`
- Each quad → 4 sub-faces, each triangle → 3 sub-faces

### Butterfly Subdivision (Interpolating)

- Regular vertices (valence = 6): classic 8-point butterfly stencil
- Extraordinary vertices: Modified Butterfly weights (trigonometric, valence-dependent)
- Original vertices are **preserved exactly** — only new edge vertices are computed

---

## 🙋 Use Cases

- 🎓 **Computer Graphics Education** — demonstrate subdivision surfaces interactively
- 🔬 **Algorithm Comparison** — real-time A/B testing of three schemes on one model
- 🛠️ **Quick 3D Preview** — load an OBJ, see how it subdivides, export the result
- 💼 **Portfolio Piece** — showcase your C++, OpenGL, and graphics knowledge in one project

---

## 📄 License

MIT © 2025

---

## ⭐ Support This Project

If this project helped you understand subdivision surfaces, visualized something beautiful, or saved your graphics assignment — **drop a Star ⭐**! It means the world. Fork it, play with it, and PRs are always welcome.
