5. Circuit Connections
MQ-2 / MQ-7 Gas Sensors:
•	VCC → 5V, GND → GND
•	AOUT → ESP32 ADC1 pin (34 / 32)
•	DOUT → ESP32 digital GPIO (35 / 33)
•	Add 100µF capacitor across VCC–GND for heater current stability
DHT11:
•	Pin 1 (VCC) → 3.3V
•	Pin 2 (DATA) → GPIO4 + 10kΩ pull-up to 3.3V
•	Pin 4 (GND) → GND
•	Pin 3 (NC) — leave unconnected
IR Flame Sensor:
•	VCC → 3.3V, GND → GND
•	DO → GPIO14 (active LOW — pull-up internally enabled)
•	AO → GPIO36 for analog intensity reading
SW-420 Vibration:
•	VCC → 3.3V, GND → GND
•	DO → GPIO27 (adjust sensitivity with onboard potentiometer)
PIR (HC-SR501):
•	VCC → 5V (note: output is 3.3V safe), GND → GND
•	OUT → GPIO26
•	Set jumper to single-trigger mode; adjust delay and sensitivity pots
Buzzer (active):
•	Drive through 2N2222 NPN BJT: base → 1kΩ → GPIO25, collector → buzzer → 5V, emitter → GND
•	Add flyback diode (1N4007) across buzzer terminals
OLED (SSD1306 I²C):
•	VCC → 3.3V, GND → GND
•	SDA → GPIO21, SCL → GPIO22
•	I²C address: 0x3C (default)
SIM800L GSM:
•	VCC → 4.2V (use LM2596 buck converter or 18650 Li-ion directly)
•	GND → GND (common with ESP32)
•	TX → GPIO16 (ESP32 UART2 RX), RX → GPIO17 (via 1kΩ + 2kΩ divider: 5V→3.3V)
