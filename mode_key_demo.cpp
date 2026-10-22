// A small example of modal keyboard controls and persistent state.

#include <algorithm>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

// The top-level selection: which set of controls owns the keyboard?
enum class ControlMode { None,
                         Intensity,
                         Geometry };

// Each mode owns its own persistent parameters.
struct IntensityState {
  bool enabled{true};
  int brightness_offset{0};  // Added to a pixel value.
};

struct GeometryState {
  bool enabled{true};
  int rotation_degrees{0};  // Rotation about an image center.
};

// One application state contains the global settings AND both mode states.
struct AppState {
  ControlMode control_mode{ControlMode::None};

  bool processing_enabled{true};  // Global switch affecting both operations.

  IntensityState intensity;
  GeometryState geometry;
};

std::string modeName(ControlMode mode) {
  switch (mode) {
    case ControlMode::None:
      return "GLOBAL";
    case ControlMode::Intensity:
      return "INTENSITY";
    case ControlMode::Geometry:
      return "GEOMETRY";
  }
  return "UNKNOWN";
}

void printControls(ControlMode mode) {
  std::cout << "\n--- " << modeName(mode) << " controls ---\n";

  switch (mode) {
    case ControlMode::None:
      std::cout << "i      Enter intensity controls\n"
                << "g      Enter geometry controls\n"
                << "p      Toggle all processing on/off\n"
                << "0      Reset ALL state to its defaults\n";
      break;

    case ControlMode::Intensity:
      std::cout << "+ / -  Increase/decrease brightness by 10\n"
                << "t      Toggle intensity processing\n"
                << "0      Reset ONLY intensity settings\n";
      break;

    case ControlMode::Geometry:
      std::cout << "+ / -  Increase/decrease rotation by 5 degrees\n"
                << "t      Toggle geometry processing\n"
                << "0      Reset ONLY geometry settings\n";
      break;
  }

  std::cout << "?      Show controls for the current mode\n"
            << "esc    Return to global controls\n"
            << "q      Quit\n"
            << "(Click the OpenCV window to give it keyboard focus; no Enter needed.)\n";
}

// Read top-level and mode-specific values without changing them.
void printState(const AppState& state) {
  const auto& intensity = state.intensity;
  const auto& geometry = state.geometry;

  // Example of how global AND local switches control effective processing.
  const bool apply_intensity = state.processing_enabled && intensity.enabled;
  const bool apply_geometry = state.processing_enabled && geometry.enabled;

  constexpr int example_pixel = 100;
  const int output_pixel = std::clamp(
      example_pixel + (apply_intensity ? intensity.brightness_offset : 0),
      0,
      255);
  const int applied_rotation =
      apply_geometry ? geometry.rotation_degrees : 0;

  std::cout << "\nMode: " << modeName(state.control_mode)
            << " | Global processing: "
            << (state.processing_enabled ? "ON" : "OFF") << '\n'
            << "  Intensity: " << (intensity.enabled ? "ON" : "OFF")
            << " | brightness offset = " << intensity.brightness_offset << '\n'
            << "  Geometry:  " << (geometry.enabled ? "ON" : "OFF")
            << " | rotation = " << geometry.rotation_degrees << " degrees\n"
            << "  Effective example: pixel 100 -> " << output_pixel
            << ", rotation = " << applied_rotation << " degrees\n";
}

// Only active in GLOBAL mode: select a mode or change global state.
void handleGlobalKey(int key, AppState& state) {
  switch (key) {
    case 'i':
      state.control_mode = ControlMode::Intensity;
      break;
    case 'g':
      state.control_mode = ControlMode::Geometry;
      break;
    case 'p':
      state.processing_enabled = !state.processing_enabled;
      break;
    case '0':
      state = AppState{};  // Reset the global AND both mode states.
      break;
    default:
      break;
  }
}

// This handler changes only the intensity substate.
void handleIntensityControlKey(int key, AppState& state) {
  auto& intensity = state.intensity;  // Reference: no copy is made.

  switch (key) {
    case '+':
      intensity.brightness_offset =
          std::clamp(intensity.brightness_offset + 10, -100, 100);
      break;
    case '-':
      intensity.brightness_offset =
          std::clamp(intensity.brightness_offset - 10, -100, 100);
      break;
    case 't':
      intensity.enabled = !intensity.enabled;
      break;
    case '0':
      intensity = IntensityState{};  // Reset intensity ONLY.
      break;
    default:
      break;
  }
}

// Reuse the SAME keys, but change the geometry substate instead.
void handleGeometryControlKey(int key, AppState& state) {
  auto& geometry = state.geometry;

  switch (key) {
    case '+':
      geometry.rotation_degrees =
          std::clamp(geometry.rotation_degrees + 5, -180, 180);
      break;
    case '-':
      geometry.rotation_degrees =
          std::clamp(geometry.rotation_degrees - 5, -180, 180);
      break;
    case 't':
      geometry.enabled = !geometry.enabled;
      break;
    case '0':
      geometry = GeometryState{};  // Reset geometry ONLY.
      break;
    default:
      break;
  }
}

// Return true whenever a mode owns the keyboard, even if its particular
// handler did not recognize the key. This prevents global fallthrough.
bool handleControlModeKey(int key, AppState& state) {
  switch (state.control_mode) {
    case ControlMode::Intensity:
      handleIntensityControlKey(key, state);
      return true;
    case ControlMode::Geometry:
      handleGeometryControlKey(key, state);
      return true;
    case ControlMode::None:
      return false;
  }
  return false;
}

// Application-wide keys get first priority; otherwise dispatch by mode.
// Return false to end the application.
bool handleKey(int key, AppState& state) {
  if (key == 'q') {
    return false;
  }
  if (key == '?') {
    printControls(state.control_mode);
    return true;
  }
  if (key == 27) {  // Escape
    state.control_mode = ControlMode::None;
    printControls(state.control_mode);
    return true;
  }

  // An active mode owns the keyboard. Do NOT fall through to global keys.
  if (handleControlModeKey(key, state)) {
    return true;
  }

  const ControlMode previous_mode = state.control_mode;
  handleGlobalKey(key, state);
  if (state.control_mode != previous_mode) {
    printControls(state.control_mode);
  }
  return true;
}

// OpenCV window: make the current settings visible while the user types.
// This is only a display; all settings remain in the AppState object.
constexpr const char* kWindowName = "Mode Key Demo";

void showStateWindow(const AppState& state) {
  cv::Mat display(350, 860, CV_8UC3, cv::Scalar(32, 32, 32));
  const cv::Scalar color(235, 235, 235);

  const auto writeLine = [&](const std::string& text, int y) {
    cv::putText(display,
                text,
                cv::Point(25, y),
                cv::FONT_HERSHEY_SIMPLEX,
                0.62,
                color,
                1,
                cv::LINE_AA);
  };

  const auto onOff = [](bool enabled) {
    return enabled ? "ON" : "OFF";
  };

  const auto& intensity = state.intensity;
  const auto& geometry = state.geometry;

  const bool apply_intensity = state.processing_enabled && intensity.enabled;
  const bool apply_geometry = state.processing_enabled && geometry.enabled;
  const int output_pixel = std::clamp(
      100 + (apply_intensity ? intensity.brightness_offset : 0), 0, 255);
  const int applied_rotation =
      apply_geometry ? geometry.rotation_degrees : 0;

  writeLine(std::string("Active controls: ") +
                modeName(state.control_mode),
            38);
  writeLine(std::string("Global processing: ") +
                onOff(state.processing_enabled),
            78);
  writeLine(std::string("Intensity: ") + onOff(intensity.enabled) +
                "  |  brightness offset: " +
                std::to_string(intensity.brightness_offset),
            113);
  writeLine(std::string("Geometry: ") + onOff(geometry.enabled) +
                "  |  rotation: " + std::to_string(geometry.rotation_degrees) +
                " degrees",
            148);
  writeLine("Effective: pixel 100 -> " + std::to_string(output_pixel) +
                ", rotation " + std::to_string(applied_rotation) + " degrees",
            183);

  cv::line(display,
           cv::Point(25, 205),
           cv::Point(830, 205),
           cv::Scalar(110, 110, 110),
           1);
  writeLine("GLOBAL: i / g select mode, p toggle processing, 0 reset all", 235);
  writeLine("IN A MODE: + / - adjust, t toggle, 0 reset current mode", 270);
  writeLine("Esc global  |  ? help  |  q quit  |  Click here to focus", 305);

  cv::imshow(kWindowName, display);
}

int main() {
  AppState state;  // One long-lived state object for the whole application.
  printControls(state.control_mode);
  printState(state);

  // cv::waitKey() needs an OpenCV HighGUI window to receive keyboard events.
  cv::namedWindow(kWindowName, cv::WINDOW_AUTOSIZE);
  showStateWindow(state);

  // 0 means wait for one keypress; there is no camera or continuous video loop.
  // The real Photo Booth uses cv::waitKey(1) in its running frame loop.
  while (true) {
    const int key = cv::waitKey(0);
    if (key < 0) {
      continue;
    }

    if (!handleKey(key, state)) {
      break;
    }
    printState(state);
    showStateWindow(state);
  }

  cv::destroyAllWindows();
  std::cout << "\nExiting\n";
}
