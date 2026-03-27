# ConsoleDash

ConsoleDash is a terminal/console Boulder Dash style game prototype written in modern C++.

It simulates a grid-based cave where rocks fall, diamonds can be collected, enemies move by rule-based AI, and explosions transform nearby tiles.  
The project is intentionally focused on engine rules and game-loop behavior rather than graphics.

## Highlights

- Boulder Dash inspired cellular update logic (top-left to bottom-right scan each tick)
- Dynamic level loading from ASCII text files
- Configurable game tick and animation tick running independently
- Animated enemies and explosion stages
- Reach mechanic (`Space` + movement key)
- Colored ANSI terminal rendering
- Animated in-game help screen
- World keeps simulating after Rockford dies until the player quits

## Project Layout

- `src/` - implementation files
- `include/` - public headers
- `levels/` - example level files
- `tests/` - minimal smoke test

## Build

From the repository root:

```bash
cmake -B build
cmake --build build
```

Run tests:

```bash
ctest --test-dir build
```

### Windows

The project compiles with G++ (MinGW/MSYS2) or MSVC on Windows without changes.

ANSI color output requires virtual terminal processing, which the game enables automatically at startup via `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)`. This works on Windows 10 1511 and later. On older systems colors will not render, but the game remains playable.

## Run

### Start with default built-in test level

```bash
./build/ConsoleDash
```

### Start with an ASCII level file

```bash
./build/ConsoleDash levels/level_1.txt
```

If the file cannot be opened, the game prints an error and exits.

## Controls

- `W`, `A`, `S`, `D` - move Rockford
- `Space` then `W/A/S/D` - reach action in that direction
- `Q` - quit

## Level File Format

Levels are plain text grids.

### Character mapping

- `@` = Rockford
- `#` = Titanium wall (indestructible)
- `W` = Wall (destructible by explosions)
- `R` = Rock
- `F` = Firefly
- `B` = Butterfly
- `A` = Amoeba
- `M` = Magic wall
- `S` = Slime
- `D` = Diamond
- `E` = Exit
- ` ` (space) = Empty space
- `.` = Dirt
- Unknown characters = Dirt

### Level parameters

After the map grid, a blank line followed by `key: value` pairs configures level behaviour:

| Parameter                | Type    | Description                                              |
|--------------------------|---------|----------------------------------------------------------|
| `NAME`                   | string  | Display name shown in the level selector                 |
| `TIME`                   | integer | Time limit in seconds                                    |
| `DIAMONDS_REQUIRED`      | integer | Diamonds needed to open the exit (default: 3)            |
| `AMOEBA_MAX_SIZE`        | integer | Maximum number of amoeba cells before it turns to rock   |
| `AMOEBA_GROWTH_FACTOR`   | integer | Growth probability denominator (higher = slower growth)  |
| `MAGIC_WALL_DURATION`    | integer | How long a magic wall stays active after first contact   |
| `SLIME_PERMEABILITY_VALUE` | integer | Permeability denominator — 0 = always passes, higher = more solid |
| `GAME_TICK_INTERVAL`     | integer | Game logic tick interval in milliseconds                 |
| `ANIMATION_TICK_INTERVAL`| integer | Animation tick interval in milliseconds                  |

### Dynamic size and limits

- Level size is inferred from file content:
  - width = longest line
  - height = number of lines
- Maximum allowed level size:
  - `x <= 100`
  - `y <= 50`
- Rendering only draws the active loaded level area (not the full max buffer).

## Gameplay Notes

- Rocks and diamonds use falling/rolling rules and can crush/trigger explosions based on falling state.
- Rockford explodes like a firefly when crushed by a falling rock or diamond.
- Fireflies and butterflies follow directional movement logic and explode under specific conditions.
- Explosions have staged states before resolving to final tiles: firefly explosions leave empty space, butterfly explosions leave diamonds.
- Magic wall converts falling rocks to diamonds and vice versa. It activates on first contact and stays active for a configurable duration. Once exhausted it cannot be reactivated, but still consumes falling objects.
- Slime is a permeable barrier. Rocks and diamonds can pass through it with a probability controlled by `SLIME_PERMEABILITY_VALUE`. Unlike magic wall it does not convert objects and is never exhausted.
- Exit display changes depending on whether required diamonds have been collected.

## Status

This is an actively evolving prototype.  
Engine behavior is being iterated toward classic Boulder Dash rules while keeping code structure clear and hackable.

