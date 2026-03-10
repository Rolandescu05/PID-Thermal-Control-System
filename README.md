# PID-Therm: Automated Temperature Control System 🌡️

An Arduino-based environment controller that uses a **PID algorithm** to maintain a target temperature with high precision. This project manages a heating element (incandescent bulb) and a cooling system (DC fan) through a full cycle: **Heating, Holding, and Cooling.**



## 🚀 Key Features
- **PID Control:** Real-time calculation of power output to minimize temperature error and overshoot.
- **Full Process Cycle:** Automated transitions between Heating, Holding, and Cooling phases.
- **On-Board Menu:** Configure target temperature (TSET), Phase Timers, and PID Constants (Kp, Ki, Kd) using 4 physical buttons.
- **LCD Interface:** Real-time monitoring of current temperature vs. setpoint.
- **Hardware Protection:** Implementation of PWM constraints and anti-windup for the integral term.

## 🛠️ Hardware Requirements
## ⚠️ Simulation vs. Physical Build
Please note that while the logic remains the same, there are key hardware differences between the **Tinkercad simulation** and the **Physical build**:

- **Temperature Sensor:** - *Simulation:* Often uses the **TMP36** (which requires a 500mV offset in code).
  - *Physical:* Uses the **LM35**, which provides a direct $10mV/°C$ output without the offset, offering better accuracy for this specific application.
- **Motor Driver:** - *Simulation:* Uses the **L293D** integrated circuit.
  - *Physical:* Uses the **L298N Module**. The physical module includes a heatsink and dedicated power terminals, which are essential for handling the current required by the incandescent bulb and fan.
- **Wiring & Pins:** Pinouts on the L298N module differ from the L293D chip. Ensure you follow the physical module's labels (ENA, IN1, IN2, etc.) as reflected in the provided code.

| **Arduino Uno** |
| **LM35** | High-precision analog temperature sensor ($10mV/°C$). |
| **L298N / L293D** | Dual H-Bridge motor driver for power management. |
| **12V Bulb** | Heating element (controlled via PWM). |
| **DC Fan** | Active cooling element. |
| **LCD 16x2** | Displays status and menu interface. |
| **4x Buttons** | Navigation (Plus, Minus, OK, Cancel). |



## 🧠 The Control Logic (PID)
The system calculates the heater power ($u(t)$) based on the error between the setpoint and current temperature:

$$u(t) = K_p e(t) + K_i \int_0^t e(\tau) d\tau + K_d \frac{de(t)}{dt}$$



- **P (Proportional):** Reacts to the current error.
- **I (Integral):** Corrects long-term steady-state errors.
- **D (Derivative):** Prevents overshoot by reacting to the rate of change.

## ⚙️ Installation & Usage
1. **Hardware:** Connect the components according to the wiring diagram from Tinkercad in the `/media` folder.
2. **Software:** Upload the `.ino` file from the `/src` folder to your Arduino Uno using the Arduino IDE.
3. **Power:** Connect a 12V power supply to the H-Bridge motor driver.
4. **Operation:** - Use **Plus/Minus** to navigate the menu.
   - Use **OK/Cancel** to adjust values.
   - Select **"Start Program"** to begin the automated cycle.

## 💡 Lessons Learned (The "Breadboard Trap")
One major takeaway from this build was discovering that some long breadboards have **split power rails**. If the sensor or buttons behave erratically, ensure the ground and 5V lines are bridged across the middle of the board. This project uses a **Common Ground** strategy to ensure signal stability between the Arduino, the H-Bridge, and the external power source.

## 📜 License
This project is licensed under the MIT License - feel free to use it and modify it for your own needs!
