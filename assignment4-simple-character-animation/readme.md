# Simple Character Animation


https://github.com/user-attachments/assets/28c24256-9efc-4935-a847-2eb810116438


## Overview

Small third-person skeletal animation demo demonstrating animation blending, root-motion sampling, and a simple third-person orbit camera.**The character model remains at the origin while the ground plane is moved to simulate locomotion.**

## Controls

- `W`: Move forward (triggers Walk/Run blending when held).
- `Left Shift` (hold): Run while moving forward.
- Mouse movement: Adjust camera pitch (Y-axis rotation is fixed).

## File Layout

- `skeletal_animation.cpp` — Main entry, input handling, animation update, rendering.
- `anim_model.vs`, `anim_model.fs` — Vertex/fragment shaders used for the skinned model.
- `ground.vs`, `ground.fs` — Shaders for the ground plane.
- `resources/objects/mixamo/warrock.dae` — Character model used by the demo.
- `resources/objects/mixamo/idle.dae`, `walk.dae`, `run.dae` — Animation clips (DAE) used for blending.
- `resources/textures/checkerboard.png` — Ground texture used for the simple plane.
- `learnopengl/` helpers — `shader_m.h`, `camera.h`, `animator.h`, `model_animation.h`, `filesystem.h`, etc.

## To run

This file need a lot of GL dependencies to run--which is not included in this repository. place this file inside the learnOpenGL repository.

## Credits

- Character model and animation clips are included under `resources/objects/mixamo/` (animations exported from Mixamo). See the repo's top-level `readme.md` or the `resources/` directory for original asset attributions and licensing details.
