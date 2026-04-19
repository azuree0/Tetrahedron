<img width="1059" height="597" alt="T" src="https://github.com/user-attachments/assets/2ac1bfa2-ea68-43d4-b7a8-79cdd29ba8ba" />

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

# Binary

```text
Artifact               # Size                        # Role             

run.exe                # 59,392 bytes    (58 KB)     # Project .cpp linked; imports SFML and OpenGL; bodies live in DLLs.
sfml-system-3.dll      # 74,240 bytes    (73 KB)     # Time, threads, UTF-8 string helpers.
sfml-window-3.dll      # 155,648 bytes   (152 KB)    # Window, events, OpenGL context creation.
sfml-graphics-3.dll    # 1,061,376 bytes (1 MB)      # Draw path, textures, text; largest runtime dependency.

                       # 1,350,656 bytes (1.29 MB)           
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
