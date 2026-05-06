# Snoopy Dash

Snoopy Dash is a Geometry Dash clone built for the M5Stack Cardputer, featuring dynamic levels, parallax scrolling backgrounds, a level designer, an inventory system, and power-ups. It supports compilation for both the Cardputer (using M5Unified and LittleFS) and a native macOS SDL-based emulator.

## Architecture & Code Organization

The monolithic `main.cpp` codebase has been refactored into a clear, modular architecture based on discrete subsystems. Due to the event-driven nature of the game and the constraints of the target hardware, a shared global state pattern is used (via `Globals.h`/`Globals.cpp`) to easily communicate data across these modules.

### Subsystems Breakdown

* **`Config.h`**
  Stores shared macro definitions, constants (like `SCREEN_WIDTH` and `CUBE_SIZE`), and cross-platform `#include`s (e.g., standardizing `millis()` and Arduino dependencies).

* **`Types.h`**
  Contains all core data structures, enums, and POD structs used throughout the game (e.g., `GameState`, `LevelData`, `PowerUpType`, `Particle`).

* **`Globals.h` & `Globals.cpp`**
  Declares and defines the shared state of the application. This includes the global state machine variable (`state`), player progress, high scores, camera positions, inventory state, and instances of rendering objects (`canvas`, `cubeSprite`, `iconSprite`).

* **`main.cpp`**
  Serves simply as the main entry point for the application. It runs initialization inside `setup()` and acts as the tick driver inside `loop()`. On macOS, it also bootstraps the M5GFX SDL wrapper.

* **`Game.h` & `Game.cpp`**
  Implements the core game loop and physics logic. This handles jump velocities, gravity application, collision detection logic, particle physics, background scrolling, and the main `update()` orchestration.

* **`Render.h` & `Render.cpp`**
  Contains all visual logic. This includes drawing the UI screens (Welcome, Level Select, Settings, Shop, Game Over) and drawing the active game frame (parallax backgrounds, tiles, spikes, and character sprites).

* **`Input.h` & `Input.cpp`**
  Handles cross-platform user input. It wraps macOS SDL keyboard polling alongside M5Cardputer rising-edge detection logic and debouncing timers, allowing gameplay code to seamlessly query input state.

* **`Audio.h` & `Audio.cpp`**
  Encapsulates music and sound effect playback. It abstracts away platform differences between ESP32 I2S MP3 playback (using `AudioGeneratorMP3`) and the macOS `afplay` command for native testing.

* **`Storage.h` & `Storage.cpp`**
  Responsible for file I/O operations. Manages the persistent storage of High Scores, player save data (lives, coins, power-ups), and parsing level JSON files using `ArduinoJson` from LittleFS or local directories. 

* **`Shop.h` & `Shop.cpp`**
  Houses the game logic for the in-game shop, which generates three random power-ups at the end of each level. It keeps track of the cost and behavior of items like the Double Jump, Big Jump, Extra Life, and Shields.

## Assets

All assets are managed within the structured `assets/` directory and synchronized automatically by the `copy_assets.py` script during the PlatformIO build process:
* `assets/audio/` - `.mp3` music tracks
* `assets/images/` - Character sprites and power-up icons
* `assets/levels/` - JSON files defining object placements and backgrounds
