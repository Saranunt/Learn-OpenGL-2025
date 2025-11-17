# Simple Plane Game

## Summary
Simple third-person airplane built on LearnOpenGL

## Demo
https://github.com/user-attachments/assets/552dd31f-ed78-4c22-b6e5-6ee4aa2b95b7

## Key Features
- Third-person chase camera that dynamically trails the aircraft.
- Lightweight flight dynamics: roll steers yaw, pitch adjusts climb/descent, and speed throttles between a configurable range.
- Procedural environment setup: large tiled water plane plus 2–4 randomly positioned islands every run.

## Controls
- `W` / `S`: Nose up / down.
- `A` / `D`: Bank left / right (roll).
- `Z` / `X`: Increase / decrease forward speed (clamped between 1–50 u/s).
- `Esc`: Quit.

## Project Layout
- `model_loading.cpp`: Main entry point, input handling, scene update, and rendering.
- `1.model_loading.*`: Shader pair for models and ground plane.
- `ground.*`: Alternate shaders for water tiling experiments.
- `resources/objects`: Plane and island meshes.
- `resources/textures`: Water texture (`wave.png`) used for the expansive ocean.

## Credits
island model:
"Green Island" (https://skfb.ly/6EMqw) by bilgehan.korkmaz is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

plane model
"Stylized Plane PFALZ D.IIIa" (https://skfb.ly/otFYJ) by ssor01 is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
