# Emit aligned rows: col1 pad W1, 10 sp, col2 pad W2, 10 sp, col3 pad W3, 10 sp, # desc
GAP = "          "
ROWS = [
    ("electron-builder.yml", "yml", "Config", "win nsis + portable → release\\"),
    ("electron.vite.config.ts", "ts", "Config", "main/preload/renderer entries"),
    ("package.json", "json", "Config", "scripts, main entry"),
    ("tsconfig.json", "json", "Config", "solution references"),
]
PREFIX, LAST = "├── ", "└── "
W1 = max(len(PREFIX) + len(r[0]) for r in ROWS)
W2 = max(len(r[1]) for r in ROWS)
W3 = max(len(f"# ({r[2]})") for r in ROWS)


def line(name, typ, role, desc, last):
    p = LAST if last else PREFIX
    c1 = p + name + " " * (W1 - len(p) - len(name))
    c2 = typ + " " * (W2 - len(typ))
    r3 = f"# ({role})" if role else ""
    c3 = r3 + " " * (W3 - len(r3))
    return c1 + GAP + c2 + GAP + c3 + GAP + "# " + desc


for i, r in enumerate(ROWS):
    print(line(r[0], r[1], r[2], r[3], i == len(ROWS) - 1))
