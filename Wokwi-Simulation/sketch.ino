
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ============================================
// 4S LITHIUM ION BATTERY MANAGEMENT SYSTEM
// ESP32 DEVKIT V1 + 16x2 I2C LCD
// WOKWI EDUCATIONAL SIMULATION
// ============================================

// WIFI CONFIGURATION
const char* ssid = "Wokwi-GUEST";
const char* password = "";

WebServer server(80);

// LCD CONFIGURATION
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================
// PIN CONFIGURATION
// ============================================

// Analog inputs - ADC1
const int cellPins[4] = {32, 33, 34, 35};

const int currentPin = 36;
const int tempPin = 39;

// Digital outputs
const int faultLED = 18;

const int balanceLED[4] = {
  13, 14, 16, 17
};

const int buzzer = 27;

// I2C
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ============================================
// SIMULATION PARAMETERS
// ============================================

// These are illustrative software thresholds.
// NOT real battery safety limits.

const float MIN_CELL_V = 2.90;
const float MAX_CELL_V = 4.25;

const float MAX_CURRENT_A = 4.50;
const float MAX_TEMP_C = 60.0;

const float BATTERY_CAPACITY_AH = 2.5;
const float CHARGE_EFFICIENCY = 0.95;

// ============================================
// GLOBAL VARIABLES
// ============================================

float cellVoltage[4] = {
  3.8, 3.8, 3.8, 3.8
};

float packVoltage = 0.0;
float current = 0.0;
float temperature = 25.0;
float SOC = 80.0;

bool voltageFault = false;
bool currentFault = false;
bool temperatureFault = false;
bool fault = false;

unsigned long lastRead = 0;
unsigned long lastSOC = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastSerialUpdate = 0;

int lcdPage = 0;

// ============================================
// READ SENSOR VALUES
// ============================================

void readSensors() {

  packVoltage = 0.0;

  // Four simulated cell voltages
  for (int i = 0; i < 4; i++) {

    int adcValue = analogRead(cellPins[i]);

    cellVoltage[i] =
      2.5 + (adcValue / 4095.0) * 1.8;

    packVoltage += cellVoltage[i];
  }

  // Current simulation: -5A to +5A
  int currentADC = analogRead(currentPin);

  current =
    (currentADC / 4095.0) * 10.0 - 5.0;

  // Positive = discharge
  // Negative = charging

  // Temperature simulation: 0 to 80 C
  int tempADC = analogRead(tempPin);

  temperature =
    (tempADC / 4095.0) * 80.0;
}

// ============================================
// PROTECTION SYSTEM
// ============================================

void checkProtection() {

  voltageFault = false;

  // Check each cell
  for (int i = 0; i < 4; i++) {

    if (cellVoltage[i] < MIN_CELL_V ||
        cellVoltage[i] > MAX_CELL_V) {

      voltageFault = true;
    }
  }

  // Check current
  currentFault =
    fabs(current) > MAX_CURRENT_A;

  // Check temperature
  temperatureFault =
    temperature > MAX_TEMP_C;

  // Overall fault
  fault =
    voltageFault ||
    currentFault ||
    temperatureFault;
}

// ============================================
// STATE OF CHARGE CALCULATION
// ============================================

void calculateSOC() {

  unsigned long now = millis();

  float elapsedHours =
    (now - lastSOC) / 3600000.0;

  lastSOC = now;

  // Positive current = discharge
  if (current > 0) {

    SOC -=
      (current * elapsedHours /
       BATTERY_CAPACITY_AH) * 100.0;
  }

  // Negative current = charging
  else if (current < 0) {

    SOC +=
      (-current * elapsedHours /
       BATTERY_CAPACITY_AH) *
      100.0 * CHARGE_EFFICIENCY;
  }

  SOC = constrain(SOC, 0.0, 100.0);
}

// ============================================
// CELL BALANCING SIMULATION
// ============================================

void balanceCells() {

  float minimumVoltage = cellVoltage[0];

  for (int i = 1; i < 4; i++) {

    if (cellVoltage[i] < minimumVoltage) {
      minimumVoltage = cellVoltage[i];
    }
  }

  for (int i = 0; i < 4; i++) {

    bool balance = false;

    if (!fault &&
        cellVoltage[i] > 3.80 &&
        cellVoltage[i] > minimumVoltage + 0.03) {

      balance = true;
    }

    digitalWrite(
      balanceLED[i],
      balance ? HIGH : LOW
    );
  }
}

// ============================================
// UPDATE OUTPUTS
// ============================================

void updateOutputs() {

  digitalWrite(
    faultLED,
    fault ? HIGH : LOW
  );

  digitalWrite(
    buzzer,
    fault ? HIGH : LOW
  );

  balanceCells();
}

// ============================================
// LCD PRINT HELPER
// Keeps each LCD line exactly 16 characters
// ============================================

void printLCDLine(int row, String message) {

  if (message.length() > 16) {
    message = message.substring(0, 16);
  }

  while (message.length() < 16) {
    message += " ";
  }

  lcd.setCursor(0, row);
  lcd.print(message);
}

// ============================================
// LCD DISPLAY SYSTEM
// Four rotating information screens
// ============================================

void updateLCD() {

  unsigned long now = millis();

  if (now - lastLCDUpdate < 2500) {
    return;
  }

  lastLCDUpdate = now;

  lcdPage = (lcdPage + 1) % 4;

  switch (lcdPage) {

    // SCREEN 1: PACK VOLTAGE AND CURRENT
    case 0:

      printLCDLine(
        0,
        "Pack:" + String(packVoltage, 2) + "V"
      );

      printLCDLine(
        1,
        "I:" + String(current, 2) + "A"
      );

      break;

    // SCREEN 2: CELL 1 AND CELL 2
    case 1:

      printLCDLine(
        0,
        "Cell 1:" + String(cellVoltage[0], 2) + "V"
      );

      printLCDLine(
        1,
        "Cell 2:" + String(cellVoltage[1], 2) + "V"
      );

      break;

    // SCREEN 3: CELL 3 AND CELL 4
    case 2:

      printLCDLine(
        0,
        "Cell 3:" + String(cellVoltage[2], 2) + "V"
      );

      printLCDLine(
        1,
        "Cell 4:" + String(cellVoltage[3], 2) + "V"
      );

      break;

    // SCREEN 4: SOC AND TEMPERATURE
    case 3:

      printLCDLine(
        0,
        "SOC:" + String(SOC, 1) + "%"
      );

      printLCDLine(
        1,
        "Temp:" + String(temperature, 1) + "C"
      );

      break;
  }
}

// ============================================
// SERIAL MONITOR
// ============================================

void printReadings() {

  Serial.println();
  Serial.println("================================");
  Serial.println("       4S BMS MONITOR");
  Serial.println("================================");

  for (int i = 0; i < 4; i++) {

    Serial.print("Cell ");
    Serial.print(i + 1);

    Serial.print(": ");
    Serial.print(cellVoltage[i], 3);
    Serial.print(" V | ");

    if (cellVoltage[i] < MIN_CELL_V) {
      Serial.println("UNDERVOLTAGE");
    }

    else if (cellVoltage[i] > MAX_CELL_V) {
      Serial.println("OVERVOLTAGE");
    }

    else {
      Serial.println("NORMAL");
    }
  }

  Serial.println("--------------------------------");

  Serial.print("Pack Voltage: ");
  Serial.print(packVoltage, 2);
  Serial.println(" V");

  Serial.print("Current: ");
  Serial.print(current, 2);
  Serial.println(" A");

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("SOC: ");
  Serial.print(SOC, 1);
  Serial.println(" %");

  Serial.println("--------------------------------");

  Serial.print("Voltage Fault: ");
  Serial.println(voltageFault ? "YES" : "NO");

  Serial.print("Current Fault: ");
  Serial.println(currentFault ? "YES" : "NO");

  Serial.print("Temperature Fault: ");
  Serial.println(temperatureFault ? "YES" : "NO");

  Serial.print("Overall Fault: ");
  Serial.println(fault ? "YES" : "NO");

  Serial.println("================================");
}

// ============================================
// JSON DATA FOR WEB DASHBOARD
// ============================================

String getJSON() {

  String json = "{";

  json += "\"packVoltage\":";
  json += String(packVoltage, 2);

  json += ",\"current\":";
  json += String(current, 2);

  json += ",\"temperature\":";
  json += String(temperature, 1);

  json += ",\"soc\":";
  json += String(SOC, 1);

  json += ",\"fault\":";
  json += (fault ? "true" : "false");

  json += ",\"voltageFault\":";
  json += (voltageFault ? "true" : "false");

  json += ",\"currentFault\":";
  json += (currentFault ? "true" : "false");

  json += ",\"temperatureFault\":";
  json += (temperatureFault ? "true" : "false");

  json += ",\"cells\":[";

  for (int i = 0; i < 4; i++) {

    if (i > 0) {
      json += ",";
    }

    json += String(cellVoltage[i], 3);
  }

  json += "]}";

  return json;
}

// ============================================
// WEB DASHBOARD
// ============================================

void handleRoot() {

  String html = R"HTML(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>4S BMS Dashboard</title>

<style>

body {
  background:#101827;
  color:white;
  font-family:Arial;
  text-align:center;
  padding:15px;
}

h1 {
  color:#38bdf8;
}

.grid {
  display:grid;
  grid-template-columns:
  repeat(auto-fit,minmax(145px,1fr));
  gap:12px;
}

.card {
  background:#1f2937;
  padding:20px;
  border-radius:12px;
}

.value {
  font-size:25px;
  font-weight:bold;
  margin:10px;
}

#status {
  padding:15px;
  border-radius:10px;
  margin:15px;
  font-weight:bold;
}

</style>

</head>

<body>

<h1>4S Battery Management System</h1>

<p>ESP32 Live Monitoring Dashboard</p>

<div id="status">Connecting...</div>

<div class="grid">

<div class="card">
Pack Voltage
<div class="value" id="v">--</div>V
</div>

<div class="card">
Current
<div class="value" id="i">--</div>A
</div>

<div class="card">
Temperature
<div class="value" id="t">--</div> C
</div>

<div class="card">
State of Charge
<div class="value" id="s">--</div>%
</div>

<div class="card">
Cell 1
<div class="value" id="c0">--</div>V
</div>

<div class="card">
Cell 2
<div class="value" id="c1">--</div>V
</div>

<div class="card">
Cell 3
<div class="value" id="c2">--</div>V
</div>

<div class="card">
Cell 4
<div class="value" id="c3">--</div>V
</div>

</div>

<script>

async function updateDashboard() {

  try {

    const response = await fetch('/data');
    const d = await response.json();

    document.getElementById('v').textContent =
      d.packVoltage.toFixed(2);

    document.getElementById('i').textContent =
      d.current.toFixed(2);

    document.getElementById('t').textContent =
      d.temperature.toFixed(1);

    document.getElementById('s').textContent =
      d.soc.toFixed(1);

    d.cells.forEach((value,index) => {

      document.getElementById('c'+index)
        .textContent = value.toFixed(3);

    });

    const status = document.getElementById('status');

    if (d.fault) {

      status.textContent = 'BATTERY FAULT DETECTED';
      status.style.background = '#dc2626';

    } else {

      status.textContent = 'BATTERY NORMAL';
      status.style.background = '#15803d';

    }

  }

  catch(error) {

    document.getElementById('status')
      .textContent = 'Waiting for ESP32...';

  }
}

setInterval(updateDashboard, 1000);
updateDashboard();

</script>

</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

// ============================================
// WEB DATA ENDPOINT
// ============================================

void handleData() {

  server.send(
    200,
    "application/json",
    getJSON()
  );
}

// ============================================
// SETUP
// ============================================

void setup() {

  Serial.begin(115200);

  analogReadResolution(12);

  // Initialize I2C LCD
  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.backlight();

  printLCDLine(0, "4S BMS SYSTEM");
  printLCDLine(1, "Initializing...");

  delay(2000);

  // Configure output pins
  pinMode(faultLED, OUTPUT);
  pinMode(buzzer, OUTPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(balanceLED[i], OUTPUT);
  }

  digitalWrite(faultLED, LOW);
  digitalWrite(buzzer, LOW);

  for (int i = 0; i < 4; i++) {
    digitalWrite(balanceLED[i], LOW);
  }

  Serial.println();
  Serial.println("Starting 4S BMS...");

  // Connect WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6);

  Serial.print("Connecting to WiFi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 15000) {

    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WiFi Connected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println();
    Serial.println("WiFi unavailable");
  }

  // Configure web server
  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();

  Serial.println("Web server started");

  lastRead = millis();
  lastSOC = millis();
  lastLCDUpdate = millis() - 2500;
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {

  server.handleClient();

  if (millis() - lastRead >= 500) {

    lastRead = millis();

    readSensors();

    checkProtection();

    calculateSOC();

    updateOutputs();

    printReadings();
  }

  // Update LCD independently
  updateLCD();
}
