<img width="950" height="587" alt="T" src="https://github.com/user-attachments/assets/07ff0bb2-ba5c-410c-8f15-0bde1de600da" />

# Prior

## Install

- **CMake 3.20+** — https://cmake.org/download/
- **C++17** (MSVC) — https://visualstudio.microsoft.com/downloads/
- **SFML 3.x** — https://www.sfml-dev.org/download.php
- **OpenGL** — `opengl32` on Windows (linked by CMake)

## Build

```
cmake -B build -DSFML_ROOT=C:/SFML
cmake --build build --config Release
```

## Run

```
.\build\Release\run.exe
```      

# Function

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│ • Tetrahedron: 3D polyhedron; 4 faces, 4 vertices, 6 edges.                │
│                                                                            │
│ • Tetractys: 10-point triangular layout (rows 1+2+3+4); classical          │       
│                                                                            │
│ • Vertex turns: 1-6 turn from tip to base.                                 │
│                                                                            │
│ • Goal: All 36 triangles show same face color.                             │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# History

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│  500 BC — Tetractys (1+2+3+4 points); Greek number emblem, not a puzzle.   │
│                                                                            │
│                                     ▼                                      │
│                                                                            │
│  360 BC — Plato *Timaeus*: tetrahedron among the regular solids.           │
│                                                                            │
│                                     ▼                                      │
│                                                                            │
│  300 BC — Euclid *Elements* XIII: the regular solids classified.           │
│                                                                            │
│                                     ▼                                      │
│                                                                            │
│  1970 — Mèffert: twistable polyhedra (vertex / edge / face mechanisms).    │
│                                                                            │
│                                     ▼                                      │
│                                                                            │
│  1981 — Pyraminx: mass-market tetrahedral puzzle (Rubik-era).              │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# Structure

```text
├── Main.cpp                        # (Frontend)          # Own window, orbit camera, HUD, and input routing.
├── Renderer.cpp                    # (Frontend)          # Draw Rubik-style stars and animated turn sweep.
├── renderer.h                      # (Frontend)          # declare stars, lighting, and mesh draw entry points.

├── Pyraminx_state.cpp              # (Backend)           # Implement Vertex And Edge Permutations For Moves.
├── pyraminx_state.h                # (Backend)           # expose colors, move types, and undo.
├── tetrahedron_mesh.cpp            # (Backend)           # build regular tetrahedron mesh and order-6 faces.
├── tetrahedron_mesh.h              # (Backend)           # hold mesh data and face centroids for rendering.
├── turn_geometry.cpp               # (Backend)           # apply rigid transforms for vertex and edge turns.
├── turn_geometry.h                 # (Backend)           # sticker shells and partial-turn geometry.
├── math3.h                         # (Backend)           # share vec3 math and axis rotation helpers.

├── CMakeLists.txt                  #                     # configure SFML 3, OpenGL, and run target.
├── copy_dlls.ps1                   #                     # copy SFML DLLs beside run.exe after build.
└── readme.md                       #                     # document install, build, run, layout, and history.
```
