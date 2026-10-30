# ESP32 4S Lithium-Ion Battery Management System (BMS)

An ESP32-based 4-Cell (4S) Lithium-Ion Battery Management System featuring real-time monitoring, passive cell balancing, protection hardware alerts, and a local web dashboard.

## Features
- **Cell Monitoring:** Tracks individual cell voltages (Cells 1–4), total pack voltage, current flow, and temperature.
- **Protection Logic:** Automates fault detection for undervoltage (<2.90V), overvoltage (>4.25V), overcurrent (>4.50A), and overtemperature (>60°C).
- **Passive Cell Balancing:** Activates individual balancing channels when cell voltage exceeds 3.80V with a delta >30mV.
- **Dual Display Interface:** Rotates telemetry data across a 16x2 I2C LCD and serves a live web dashboard (`/data` AJAX endpoint).
- **State of Charge (SOC):** Dynamic Coulomb-counting algorithm with charging efficiency compensation.

## Hardware & Pinout
| Function | Pin / Resource |
| :--- | :--- |
| **Cell Voltages (1–4)** | GPIO 32, 33, 34, 35 |
| **Current / Temp** | GPIO 36 (ADC1), GPIO 39 (ADC1) |
| **I2C LCD (16x2)** | SDA: GPIO 21, SCL: GPIO 22 |
| **Balancing LEDs (1–4)** | GPIO 13, 14, 16, 17 |
| **Fault LED / Buzzer** | GPIO 18 / GPIO 27 |

## Simulation
Try the project live on Wokwi: [ESP32 4S BMS Wokwi Simulation](https://wokwi.com/projects/479229648134279169)