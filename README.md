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
- `E` = Exit
- ` ` (space) = Empty space
- `.` = Dirt
- Unknown characters = Dirt

`D` is also accepted as a diamond in current implementation for compatibility with existing level examples.

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
- Fireflies and butterflies follow directional movement logic and explode under specific conditions.
- Explosions have staged states (`a`, `b`, `c`) before resolving to final tiles.
- Exit display changes depending on whether required diamonds have been collected.

## Status

This is an actively evolving prototype.  
Engine behavior is being iterated toward classic Boulder Dash rules while keeping code structure clear and hackable.

