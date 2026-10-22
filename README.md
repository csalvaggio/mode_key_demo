# Modal keyboard controls and persistent state

## Purpose

The instructor's Photo Booth application uses *control modes*: the meaning of a
keyboard key depends on which set of controls is active. This standalone
example demonstrates the same architecture with two processing modes:
**Intensity** and **Geometry**.

It intentionally does not include a camera or image-processing algorithms.

OpenCV is used only to create a small status window and read individual
keypresses with `cv::waitKey()`. The focus remains keyboard dispatch and
storage of parameters, not the algorithms that consume those parameters.

This exercise demonstrates four separate ideas:

1. An enumeration identifies the current keyboard control mode.
2. A separate structure stores parameters for each processing mode.
3. A top-level structure contains both sets of parameters and global settings.
4. A single keyboard dispatcher routes keys to the appropriate handler.

## Build and run

You need a C++20 compiler, CMake, and an OpenCV installation that includes
the `core`, `highgui`, and `imgproc` modules (the same OpenCV components used
by the Photo Booth).

```sh
cmake -S . -B build
cmake --build build
./build/mode_key_demo
```

**Input note:** Click the OpenCV window to give it keyboard focus, then press
the keys directly (no Enter key is required). `Esc` is the actual Escape key,
not the letters `e`, `s`, and `c`. The window displays current values, and
additional details are printed in the terminal. Use `q` to quit.

**Why `cv::waitKey(0)`?** There is no camera or continuously updated frame in
this small example, so each call simply waits until a key is pressed. An
application that is continuously processing frames typically uses
`cv::waitKey(1)` inside its frame loop. The key dispatch functions are the
same in both cases.

## Keyboard controls

| Context | Key | Action |
|---|---|---|
| Global | `i` | Enter Intensity mode |
| Global | `g` | Enter Geometry mode |
| Global | `p` | Toggle global processing (affects both operations) |
| Global | `0` | Restore **all** application state to defaults |
| Intensity | `+` / `-` | Adjust brightness offset by +10 / -10 |
| Intensity | `t` | Toggle intensity processing |
| Intensity | `0` | Reset **only** intensity settings |
| Geometry | `+` / `-` | Adjust rotation by +5 / -5 degrees |
| Geometry | `t` | Toggle geometry processing |
| Geometry | `0` | Reset **only** geometry settings |
| Any | `?` | Show controls for the current context |
| Any | `esc` | Return to Global mode **without resetting parameters** |
| Any | `q` | Quit |

**Important:** To switch directly from Intensity to Geometry, first return to
Global with `esc`, then choose `g`. Keys belonging to Global mode are not active
inside Intensity or Geometry. The only exceptions are intentionally
application-wide keys (`?`, `esc`, `q`).

## State hierarchy

```text
AppState
|-- control_mode           <-- Which handler receives keyboard input?
|-- processing_enabled     <-- Global setting affecting BOTH operations
|-- intensity              <-- IntensityState
|   |-- enabled
|   `-- brightness_offset
`-- geometry               <-- GeometryState
    |-- enabled
    `-- rotation_degrees
```

The entire state is created once in `main()`:

```cpp
AppState state;
```

Each call to `handleKey(key, state)` passes this **same object by reference**.
Its member variables therefore retain their values from one keypress to the
next. Switching `state.control_mode` does **not** construct a new `AppState`,
`IntensityState`, or `GeometryState`.

## How key reuse works

The two mode handlers deliberately recognize overlapping keys:

```cpp
void handleIntensityControlKey(int key, AppState& state) {
  auto& intensity = state.intensity;
  // '+' increases intensity.brightness_offset.
}

void handleGeometryControlKey(int key, AppState& state) {
  auto& geometry = state.geometry;
  // '+' increases geometry.rotation_degrees.
}
```

The central `handleKey()` first processes keys that work everywhere (`?`,
`esc`, `q`). It then calls `handleControlModeKey()`, which selects **one** of the two mode handlers. Only when the current mode is `None` does it call
`handleGlobalKey()`.

`handleControlModeKey()` returns `true` for *any* key received while a mode is
active, even when the mode handler does not recognize that key. In this
example, `true` means **a mode had ownership of the key**, not necessarily
that it performed an operation. This distinction stops an unrecognized key
from unintentionally triggering a global action.

## Reading and updating global and local values

Within a mode handler, use a **non-const reference** to modify persistent
parameters:

```cpp
auto& intensity = state.intensity;
intensity.brightness_offset += 10;
```

The top-level setting is available from the same `state` object:

```cpp
bool apply_intensity =
    state.processing_enabled && state.intensity.enabled;
```

For a function that only *reads* state, pass it as a **const reference**:

```cpp
void printState(const AppState& state);
```

Notice how `printState()` reads `state.control_mode`,
`state.processing_enabled`, `state.intensity.brightness_offset`, and
`state.geometry.rotation_degrees` together. In a real processing pipeline,
those reads would determine which operations to apply to an image.

In this demonstration, the printed example calculates an adjusted pixel value
starting at 100 and reports the active rotation angle. It **does not** rotate
an actual image; the OpenCV window simply displays the stored settings.

## Try this sequence

Click the OpenCV window and press the following keys in order:

```text
i
+
+
esc
g
+
esc
i
```

The final state will still have **brightness offset = 20** and **rotation =
5 degrees**. Entering a mode does not reset the parameters of that mode.

Now experiment with these differences:

- In Intensity mode, `0` resets the brightness offset but leaves rotation alone.
- In Global mode, `0` resets **all** application state.
- In Global mode, `p` disables the *application* of both effects but preserves
  each mode's stored values. Press `p` again to re-enable processing.
- Try pressing `p` while Intensity or Geometry is active. Nothing happens; it
  cannot fall through to `handleGlobalKey()`.

## Applying this design to the Photo Booth

The architectural correspondence is direct:

| Tutorial | Instructor Photo Booth |
|---|---|
| `ControlMode` | `ControlMode` |
| `AppState` | `ProcessingState` |
| `IntensityState` | Intensity-related members of `ProcessingState` |
| `GeometryState` | `GeometricProcessingState` |
| `handleGlobalKey()` | `handleGlobalKey()` |
| `handleControlModeKey()` | `handleControlModeKey()` |
| `handleKey()` | `handleKey()` |

The Photo Booth has more modes, more data, and some processing-related
objects kept separately from `ProcessingState`. The *keyboard ownership
pattern* is the same. The image processing code reads the stored settings
repeatedly while it processes incoming frames; the keyboard functions change
settings as events arrive.

The tutorial and Photo Booth now both acquire individual keys using
`cv::waitKey()`. Their principal difference is the delay:

```cpp
// Tutorial: there is no camera, so wait for the next keypress.
const int key = cv::waitKey(0);

// Photo Booth: check for a key while the live frame loop continues.
const int key = cv::waitKey(1);
```

The key-acquisition line produces an integer that is passed to the same
mode dispatcher:

```cpp
if (!handleKey(key, state)) {
  break;
}
```

The teaching version of `handleKey()` takes two arguments. The production
Photo Booth handler has additional parameters because it also manages other
application features; keep those parameters when adapting the pattern.

The display window is just an observer: `showStateWindow(const AppState&)`
reads state but does not modify it. The state still lives in `main()` and is
modified by the keyboard handlers through non-const references.

## License

This project is licensed under the GNU General Public License v3.0.
See `LICENSE` for details.

## Contact

### Author

Carl Salvaggio, Ph.D.  
Professor of Imaging Science  
Director, Digital Imaging and Remote Sensing (DIRS) Laboratory

### E-mail

carl.salvaggio@rit.edu

### Organization

Chester F. Carlson Center for Imaging Science  
Rochester Institute of Technology  
Rochester, New York, 14623  
United States
