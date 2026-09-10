# Flappy Bird (raylib C Implementation) Documentation

A 2D arcade game clone of Flappy Bird written in C utilizing the **raylib** graphics and audio library. This document outlines the architecture, data structures, execution lifecycle, rendering algorithms, and mathematical models implemented across `game.c`.

---

## 1. Architectural Overview & Configuration

The application operates as a single-threaded, frame-driven game loop targeting 60 frames per second with VSync enabled. State management relies on explicit delta-time physics integration ($dt$), procedural obstacle generation with object pooling/recycling, and layered parallax 2D rendering.

### Display & Screen Dimensions
* `WIDTH`: `1080` px (Viewport width)
* `HEIGHT`: `720` px (Viewport height)
* `DEBUG`: `0` (Debug mode toggle)
* `HITBOX`: `1` (Renders visual bounding boxes for bird and pipe clearances)

### Gameplay & Kinematics Parameters
* `gravity`: `1000.0f` px/s² (Downwards vertical acceleration)
* `flap_velocity`: `-425.0f` px/s (Upwards impulse upon input)
* `game_speed`: `300.0f` px/s (Leftward linear velocity for pipes and ground terrain)
* `background_speed`: `70.0f` px/s (Parallax background scroll rate)
* `pipe_vertical_distance`: `200.0f` px (Clearance gap height between top and bottom pipes)
* `pipe_horizontal_distance`: `400.0f` px (Horizontal spacing between successive pipe instances)
* `rotation_speed`: `75.0f` deg/s (Pitch rotation base modifier)

---

## 2. Core Data Structures

### `Pipe`
Represents an active obstacle pair on screen.
```c
typedef struct {
    float x;      // Horizontal position of the pipe pair
    float y;      // Vertical Y-coordinate representing the top edge of the gap
    int passed;   // Flag: 0 = unpassed, 1 = scored past the player's X coordinate
} Pipe;
```

### `Sfx`
Audio handle container aggregating sound wave buffers:
```c
typedef struct {
    Sound flap;   // audio/wing.wav - Played on wing flap impulse
    Sound death;  // audio/die.wav  - Played on lethal collision
    Sound point;  // audio/point.wav - Played upon passing pipe threshold
    Sound hit;    // audio/hit.wav   - Loaded collision impact SFX
} Sfx;
```

### `Scale`
Defines rendering scale factors across game elements:
```c
typedef struct {
    Vector2 bird;        // (2.0, 2.0)
    Vector2 pipe;        // (1.5, 2.0)
    Vector2 background;  // Scaled dynamically: HEIGHT / background.height
    Vector2 ground;      // (1.0, 1.0)
} Scale;
```

### `Animation` & `Assets`
Predefined schema declarations designed to bundle sprite sheets and multi-frame animation states.

---

## 3. Execution Lifecycle & Main Loop

### Initialization Flow (`main`)
1. **Window & Audio Setup**:
   * Initializes raylib window context via `InitWindow(1080, 720, "Flappy Bird")`.
   * Sets audio context via `InitAudioDevice()`.
   * Sets target frame rate to 60 FPS (`SetTargetFPS(60)`) and sets `FLAG_VSYNC_HINT`.
   * Sets window icon to `sprites/yellowbird-midflap.png`.
2. **Persistence**:
   * Calls `load_and_save_high_score(0, 'l')` to retrieve persistent record from `high_score.txt`.
3. **Asset Loading**:
   * Calls `load_textures()` to load PNG textures into GPU memory.
   * Calls `load_sound()` to populate SFX structures.
4. **Entity Initialization**:
   * Computes dynamic pipe buffer size: `total_pipes = WIDTH / pipe_horizontal_distance + 1` (4 pipes).
   * Spawns pipes off-screen with randomized gap intervals using `init_pipes()`.

### Game Loop States
* **Active Gameplay State (`!game_over`)**:
  1. Computes delta time: `dt = GetFrameTime()`.
  2. Updates bird kinematic properties (`update_velocity`, `move_bird`).
  3. Updates pipe translations and recycling (`move_pipe`).
  4. Runs collision detection (`check_death`).
  5. Computes scoring metrics (`update_score`) and updates high scores.
  6. Draws layered rendering buffer (Background $	o$ Bird $	o$ Pipes $	o$ Ground $	o$ UI/HUD).
* **Game Over State (`game_over == 1`)**:
  * Freezes positional calculations; retains last known entity coordinates.
  * Draws stationary background, pipes, bird, ground, score overlay, and `gameover.png` banner.
  * Awaits reset trigger (`KEY_SPACE` or mouse left button) to reset position, velocities, score, and reinitialize pipe positions.

### Teardown
* Saves the session's high score to `high_score.txt` via `load_and_save_high_score(high_score, 's')`.
* Invokes `free_memory()` to unload GPU textures and sound buffers.
* Closes audio device (`CloseAudioDevice()`) and window context (`CloseWindow()`).

---

## 4. Subsystem & Function Documentation

### Kinematics & Physics Integration

#### `float update_velocity(float velocity, float dt)`
* **Mathematical Formula**:
  $$\Delta v = g \cdot dt \implies v_{t} = v_{t-1} + g \cdot dt$$
* If `KEY_SPACE` or `MOUSE_BUTTON_LEFT` is registered:
  * Overrides current vertical velocity with `flap_velocity` (`-425.0f`).
  * Triggers `sfx.flap`.
  * Resets pitch angle `bird_rotation = -30.0f`.

#### `float move_bird(float pos_y, float velocity, float dt)`
* Computes inverted-coordinate vertical translation:
  $$y_{t} = y_{t-1} + v_{t} \cdot dt$$

#### `void draw_bird(int x, int y, float velocity)`
* Computes animation frame index cyclically based on elapsed game time:
  $$\text{frame\_no} = \left\lfloor \frac{\text{GetTime}()}{\text{animation\_time} / 3} \right\rfloor \pmod 3$$
* Applies non-linear angular acceleration to emulate nose-dive rotation using piecewise checks and logarithmic progression.
* Renders bird sprite anchored at its geometric center via `DrawTexturePro`.

---

### Procedural Pipe Management & Recycling

#### `void init_pipes(Pipe pipes[], int total_pipes)`
* Initializes horizontal positions spaced evenly:
  $$\text{pipes}[i].x = \text{WIDTH} + (i + 1) \cdot \text{pipe\_horizontal\_distance}$$
* Sets gap offset randomly:
  $$\text{pipes}[i].y = (\text{rand}() \pmod{\text{HEIGHT} - 400}) + 50$$

#### `void move_pipe(Pipe pipes[], int total_pipes, float dt)`
* Translates all pipes leftward:
  $$\text{pipes}[i].x \gets \text{pipes}[i].x - \text{game\_speed} \cdot dt$$
* Detects when a pipe exits screen boundaries (`pipe.x + pipe_width * 2 <= 0`).
* Repositions the expired pipe behind the current furthest pipe:
  $$\text{pipe}_{\text{recycled}}.x = \max(\mathbf{x}) + \text{pipe\_horizontal\_distance}$$
  with a new randomized vertical gap interval and resets `passed = 0`.

#### `void draw_pipes(Pipe pipes[], int total_pipes)`
* **Bottom Pipe**: Rendered straight from texture origin.
* **Top Pipe**: Shifted by clearance distance, origin anchored at center, rotated $180^\circ$ to render upside-down above the gap.

---

### Collision Detection & Scoring

#### `int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes)`
Performs Axis-Aligned Bounding Box (AABB) intersection tests:
1. **Ceiling Collision**: $pos_y - \frac{h}{2} \le 0$
2. **Ground Collision**: $pos_y + \frac{h}{2} \ge \text{HEIGHT} - \text{ground.height}$
3. **Obstacle Overlap**:
   Checks if the bird's horizontal footprint overlaps with the pipe column:
   $$\text{pipe}.x \le pos_x + \frac{w}{2} \quad \land \quad \text{pipe}.x + w_{\text{pipe}} \ge pos_x - \frac{w}{2}$$
   If true, flags collision if:
   $$pos_y - \frac{h}{2} \le \text{pipe}.y \quad (\text{Top pipe hit})$$
   $$pos_y + \frac{h}{2} \ge \text{pipe}.y + \text{pipe\_vertical\_distance} \quad (\text{Bottom pipe hit})$$

#### `int update_score(int score, float bird_x, int total_pipes, Pipe pipes[])`
Checks if `pipes[i].x <= bird_x` and `!pipes[i].passed`. Increments score by 1, flags `pipes[i].passed = 1`, and plays `sfx.point`.

---

### Parallax Background & Endless Ground Scrolling

#### `draw_background(void)` & `draw_ground(void)`
* Computes horizontal tiling count required to exceed window dimensions:
  $$N = \left\lfloor \frac{\text{WIDTH}}{\text{texture\_width} \cdot \text{scale}} \right\rfloor + 2$$
* Blits adjacent textured quads offset by `backgroung_position` and `base_poition`.
* Loops offset modulo tile width to ensure seamless infinite scrolling.

---

### Persistent Storage

#### `int load_and_save_high_score(int high_score, char load_or_save)`
* **Read Mode (`'l'`)**: Opens `high_score.txt`, reads integer value via `fscanf`, closes file stream, and returns score.
* **Write Mode (`'s'`)**: Opens `high_score.txt` in write mode (`"w"`), outputs formatted integer via `fprintf`, and flushes stream.

---

## 5. Asset Manifest

| Asset Path | Type | Usage |
| :--- | :--- | :--- |
| `sprites/yellowbird-downflap.png` | PNG Texture | Frame 0 of bird flap animation cycle |
| `sprites/yellowbird-midflap.png` | PNG Texture | Frame 1 of bird flap animation cycle / Window Icon |
| `sprites/yellowbird-upflap.png` | PNG Texture | Frame 2 of bird flap animation cycle |
| `sprites/pipe-green.png` | PNG Texture | Obstacle pipe texture (rendered normal and flipped $180^\circ$) |
| `sprites/background-day.png` | PNG Texture | Parallax background layer |
| `sprites/base.png` | PNG Texture | Scrolling ground terrain |
| `sprites/gameover.png` | PNG Texture | Modal Game Over display badge |
| `audio/wing.wav` | WAV Audio | Flap audio trigger |
| `audio/die.wav` | WAV Audio | Player death trigger |
| `audio/point.wav` | WAV Audio | Score threshold trigger |
| `audio/hit.wav` | WAV Audio | Bounding collision trigger |
