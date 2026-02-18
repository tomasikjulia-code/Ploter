# STM32 Powered DIY Pen Plotter (CNC)

A custom-built, 2-axis CNC Pen Plotter designed for precision drawing and sketching. This project features a standalone control system based on the **STM32 (Blue Pill)**, allowing for G-code execution directly from an SD card without the need for a constant PC connection.

## Features

* **Standalone Operation:** Equipped with a Micro SD card module and a 1.3" OLED display for file selection and real-time status monitoring.
* **STM32 Core:** Powered by the STM32F103C8T6 (Blue Pill), providing higher processing power and more memory than standard Arduino-based plotters.
* **Mechanical Precision:** Uses NEMA 17 stepper motors and lead screws for the X and Y axes, ensuring high torque and accuracy.
* **Custom Electronics:** Hand-soldered prototype board integration with dedicated motor drivers and a buck converter for stable power management.
* **Hybrid Build:** A robust frame combining 3D-printed components, aluminum profiles, and wooden supports.

---

## Hardware Components

| Category | Component |
| :--- | :--- |
| **Microcontroller** | STM32F103C8T6 (Blue Pill) |
| **Display** | 1.3" I2C OLED Display |
| **Storage** | Micro SD Card Module (SPI) |
| **Motion** | 2x NEMA 17 Stepper Motors + Lead Screws |
| **Drivers** | A4988 Stepper Drivers |
| **Power** | DC Jack Input + Buck Converter (Step-down to 5V/3.3V) |
| **Pen Lift** | Micro Servo (MG90) |

---

## How It Works

1.  **G-Code Generation:** Create designs in Inkscape or LaserWeb and export them as `.txt` files.
2.  **Offline Printing:** Save the files onto a Micro SD card and insert it into the plotter's control box.
3.  **Interface:** Use the onboard buttons and OLED menu to navigate the SD card and select the file you wish to draw.
4.  **Execution:** The STM32 parses the G-code and coordinates the stepper motors and the pen-lift servo.

---

<p align="center">
  <img src="zdjecia/zdjęcia plotera.jfif" width="600" title="Mechanical Construction">
</p>


