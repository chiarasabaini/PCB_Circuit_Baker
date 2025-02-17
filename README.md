# P.C.B. - PCB Circuit Baker

---

## Table of Contents
- [Project Overview](#project-overview)
- [See more](#see-more)
- [Project structure](#project-structure)
- [Core Functionality](#core-functionality)
- [Features](#features)
- [Usage](#usage)
- [Software Requirements](#software-requirements)
- [Hardware Requirements](#hardware-requirements)
- [Build and Run](#build-and-run)
- [Contribution](#contribution)

---

## Project Overview

This project aims to transforms a small electric pizza oven, into a functional pcb oven for reflow operations. It leverages on a custom board based on the STM32F030 microcontroller, with integrated power supply, thermocoupler conditioners and SSR driving stages.

---

## See more

- [Pitch video](https://www.youtube.com)
- [Presentation](https://docs.google.com/presentation/d/1FvlHxZEMd3MMXUBN6C8jSyhKtvd0TKJ8RmZ0mUIsl6k/edit?usp=sharing)

---

## Project structure

```
PCB-Circuit-Baker
├── Firmware
└── README.md
```

---

### Core Functionality

- **STM32F030CCT6**:
  - generates temperature profile, based on user input
  - reads temperature from type K thermocouples
  - modulates power to heating elements
  - handles user interaction with the buttons board 



---

### Features

- **Mode of Operation**
  - **static mode**: lets user select a temperature to keep.

  - **temperature control**: lets user create a temperature to follow over a time period: profiles are saved

- **PID Parameter**
  - tunable PID parameter
  - parameter are saved

- **LCD Display**:
  - serves as GUI for the user to interact with the oven: displays profile and current temperature

- **User Board**:
    - made of tactile switches 

---

## Usage

1. Connect the oven to main line
2. (optional) Adjust parameters
3. Select the desired mode of operation
4. Insert the pcb
5. Press the start button
6. Wait for the process to finish
7. Remove the pcb
8. Enjoy your pizza!

---
  
## Software Requirements

To use this project, you will need the following software:

- OpenOCD
- ARM toolchain

## Hardware Requirements

To use this project, you will need the following hardware:

- pizza oven
- depending on heating capabilities of the oven, more heating elements
- fully assembled controller board
- fully assembled user input board
- fully assembled display board
- 2 type K thermocouple
- 2 Solid State Relay, with correct power rating
- wires, metal sheet scraps and insulating material
- a bit of ingenuity to integrate all components togheter

### Schematic

<img width="1672" alt="image" src="https://github.com/user-attachments/assets/6754baf5-04f9-46c7-9865-6e7bbe14c434" />

---

## Build and Run

git clone git@github.com:chiarasabaini/PCB_Circuit_Baker.git
make flash -j
---

## Contribution

- Aris Tomaselli
- Chiara Sabaini
- Silvia Bragantini
- Andrea Aldeni

> This project is a collaborative effort by all team members. We adopted a pair programming approach, frequently meeting to work together. Tasks were shared among us, and we supported each other in solving any challenges that arose.
