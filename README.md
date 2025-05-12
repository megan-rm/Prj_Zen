
# Zen

**Zen** is a side-view simulated ecosystem built using **C++17** and **SDL2**. It simulates water cycles, weather patterns, plant growth, and animal interactions in a pixel-art sandbox environment. Every world is unique and driven by emergent behavior, not predefined scripts.

---

## Features

- **Tile-Based World**  
  Worlds are made of 8×8 pixel tiles with dynamic properties like permeability, saturation, and temperature.

- **Water Simulation**  
  Rain falls, saturates the soil, and evaporates over time, forming clouds. Moisture moves realistically through the environment.

- **Humidity and Clouds**  
  Tiles track humidity levels and spawn cloud puffs dynamically. Cloud rendering adapts based on view and density.

- **Adaptive Life** *(Work in Progress)*  
  Simulated flora and fauna react to environmental conditions, including water availability, weather, and temperature.

---

## Screenshot
![Alt text](/screenshots/screenshot1.png?raw=true "Night Time")
![Alt text](/screenshots/screenshot2.png?raw=true "Dynamic Humidity Map")
![Alt text](/screenshots/screenshot3.png?raw=true "dynamic heat system and heat map")
![Alt text](/screenshots/screenshot4.png?raw=true "Evening clouds, pushed by the wind")

---

## Built With

- **C++17**
- **SDL2** – Simple DirectMedia Layer 2

---

## Development Status

Zen is currently in **active development**. The simulation engine is functional, with performance optimizations and life simulation in progress.

---

### Dependencies

- SDL2
- SDL2_image

---

## Goals

- Emphasize **emergent behavior** from simple rules
- Avoid scripting or hard-coded logic for life and environment simulation

---

## Philosophy

"**Simplicity is the ultimate sophistication.**" – Leonardo da Vinci

Zen strives for quiet complexity. It’s a place where rain falls, plants grow, and life happens—without intervention.
