# Emit 3-column Structure tree: col1 files, 10 spaces, col2 # (Role), 10 spaces, col3 # Description
GAP = 10
ROWS = [
    ("├── CMakeLists.txt", "# (Config)", "# SFML 3, OpenGL, target run"),
    ("├── copy_dlls.ps1", "# (Config)", "# Copy SFML DLLs beside run.exe"),
    ("├── main.cpp", "# (Frontend)", "# window, orbit, HUD, input"),
    ("├── math3.h", "", "# Vec3 and axis rotations"),
    ("├── tetrahedron_mesh.h", "# (Backend)", "# mesh and centroids"),
    ("├── tetrahedron_mesh.cpp", "# (Backend)", "# regular tet + order-6 faces"),
    ("├── turn_geometry.h", "# (Backend)", "# sticker shells + partial turns"),
    ("├── turn_geometry.cpp", "# (Backend)", "# vertex / edge rigid transforms"),
    ("├── pyraminx_state.h", "# (Backend)", "# colors, moves, undo"),
    ("├── pyraminx_state.cpp", "# (Backend)", "# vertex / edge permutations"),
    ("├── renderer.h", "# (Frontend)", "# stars, lighting, mesh draw"),
    ("├── renderer.cpp", "# (Frontend)", "# Rubik-style stars + turn sweep"),
    ("└── README.md", "", "# project readme"),
]
w1 = max(len(a) for a, _, _ in ROWS)
w2 = max((len(b) for _, b, _ in ROWS if b), default=0)
w2 = max(w2, len("# (Frontend)"))
for a, b, c in ROWS:
    print(a.ljust(w1) + (" " * GAP) + b.ljust(w2) + (" " * GAP) + c)
