/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <ESP8266WiFi.h>

// Replace with your network credentials
const char* ssid     = "Bratlol";
const char* password = "Hello123";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
String output5LongPressAction = "none"; // To store feedback for long press
String output4LongPressAction = "none"; // To store feedback for long press
String lastDirectionalAction = "none"; // For new directional controls


char digit1;
char digit2;
char digit3;
char digit4;

// ---- NEW: Global variable for speed ----
int currentSpeed = 150; // Default speed (0-500, adjust range as needed for your motors)

#define clamp(value, min, max) (value < min ? min : value > max ? max : value)

void setup() {

  int state = LOW;
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);

  // Connect to Wi-Fi network with SSID and password
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    state = toggle(state);
    digitalWrite(output4, state);
    //Serial.print(".");
    //Serial.println("PWSTANDBY");
  }
  // Print local IP address and start web server
  //Serial.println("");
  //Serial.println("WiFi connected.");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
  digitalWrite(output4, LOW);
  digitalWrite(output5, HIGH);
  server.begin();
  
}

uint8_t toggle(uint8_t state)
{
  if(state == LOW)
  {
    state = HIGH;
  }
  else
  {
    state = LOW;
  }

  return state;
  
}

void drive(int velocity, int radius)
{
  clamp(velocity, -500, 500); //def max and min velocity in mm/s
  clamp(radius, -2000, 2000); //def max and min radius in mm
  
  Serial.write(137);
  Serial.write(velocity >> 8);
  Serial.write(velocity);
  Serial.write(radius >> 8);
  Serial.write(radius);
}

void driveWheels(int right, int left)
{
  clamp(right, -500, 500);
  clamp(left, -500, 500);
  
  Serial.write(145);
  Serial.write(right >> 8);
  Serial.write(right);
  Serial.write(left >> 8);
  Serial.write(left);
  }

//---------------------------------------------------------------
void driveStop(void)
{
  drive(0,0);
}

//---------------------------------------------------------------
void driveLeft(int left)
{
  driveWheels(left, 0);
}

//---------------------------------------------------------------
void driveRight(int right)
{
  driveWheels(0, right);
}

void reset(void)
{
  Serial.write(7);
}

/*--------------------------------------------------------------------------
This command powers down Roomba.*/
void powerOff(void)
{
  Serial.write(133);
}

void playSound (int num) 
{
  switch (num)
  { 
    case 1: 
      Serial.write("\x8c\x01\x04\x42\x20\x3e\x20\x42\x20\x3e\x20"); // [140] [1] [4] [68] [32] ... place "start sound" in slot 1
      Serial.write("\x8d\x01"); // [141] [1] play it (in slot 1)
      break;
 
    case 2: 
      Serial.write("\x8c\x01\x01\x3c\x20"); // place "low freq sound" in slot 2
      Serial.write("\x8d\x01"); // play it (in slot 2)
      break;

    case 3:
      Serial.write("\x8c\x01\x01\x48\x20"); // place "high freq sound" in slot 3
      Serial.write("\x8d\x01"); // play it (in slot 3)
      break;
  }
}

/*--------------------------------------------------------------------------
This command controls the four 7 segment displays on the Roomba using ASCII character codes.*/
void setDigitLEDFromASCII(byte digit, char letter)
{
  switch (digit){
  case 1:
    digit1 = letter;
    break;
  case 2:
    digit2 = letter;
    break;
  case 3:
    digit3 = letter;
    break;
  case 4:
    digit4 = letter;
    break;
  }
  Serial.write(164);
  Serial.write(digit1);
  Serial.write(digit2);
  Serial.write(digit3);
  Serial.write(digit4);
}

void writeLEDs (char a, char b, char c, char d)
{
  setDigitLEDFromASCII(1, a);
  setDigitLEDFromASCII(2, b);
  setDigitLEDFromASCII(3, c);
  setDigitLEDFromASCII(4, d);
}



/*--------------------------------------------------------------------------
This command gives you complete control over Roomba by putting the OI into Full mode, and turning off the cliff, wheel-drop and internal charger safety features.*/
void startFull()
{  
  Serial.write(128);  //Start
  Serial.write(132);  //Full mode
  delay(1000);
}

/*--------------------------------------------------------------------------
This command stops the OI. All streams will stop and the robot will no longer respond to commands. Use this command when you are finished working with the robot. */
void stop(void)
{
  Serial.write(173);
}


// --- Inside parseSpeedFromHeader ---
int parseSpeedFromHeader(const String& reqHeader, int defaultSpeed) {
    int speedVal = defaultSpeed;
    int speedParamIdx = reqHeader.indexOf("?speed=");
    if (speedParamIdx == -1) {
        speedParamIdx = reqHeader.indexOf("&speed=");
    }

    if (speedParamIdx != -1) { // Changed from > 0 for robustness
        String speedStr = "";
        int valStartIndex = speedParamIdx + 7;
        for (int i = valStartIndex; i < reqHeader.length(); i++) {
            char ch = reqHeader.charAt(i);
            if (isdigit(ch)) {
                speedStr += ch;
            } else {
                break;
            }
        }
        if (speedStr.length() > 0) {
            int parsed = speedStr.toInt();
            if ((parsed != 0 || speedStr.equals("0")) && parsed >= 0 && parsed <= 500) {
                speedVal = parsed;
            } 
        } 
    } 

    return speedVal;
}


void loop() {
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) { // If a new client connects,
    //Serial.println("New Client.");
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;
    header = ""; // Clear header for new request

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) { // End of HTTP request headers
            //Serial.println("Request Header Received:");
            //Serial.println(header);

            bool actionTaken = false;
            int requestedSpeed = currentSpeed; // Use currentSpeed by default

            // --- Handle GPIO state changes based on request ---
            // GPIO 5
            if (header.indexOf("GET /5/on") >= 0 && header.indexOf("GET /5/on_long") < 0) {
              //Serial.println("GPIO 5 ON (Short press)");
              output5State = "on"; digitalWrite(output5, HIGH); output5LongPressAction = "Full On"; actionTaken = true;
              startFull();
              writeLEDs ('b', 'e', 'r', 't');
            } else if (header.indexOf("GET /5/off") >= 0 && header.indexOf("GET /5/off_long") < 0) {
              //Serial.println("GPIO 5 OFF (Short press)");
              output5State = "off"; digitalWrite(output5, LOW); output5LongPressAction = "Stopped System"; actionTaken = true;
              stop();
              writeLEDs ('S', 'T', 'O', 'P');
            } else if (header.indexOf("GET /5/on_long") >= 0) {
              //Serial.println("GPIO 5 ON (Long press)");
              output5State = "on"; digitalWrite(output5, HIGH); output5LongPressAction = "reset system"; actionTaken = true;
              reset();
            } else if (header.indexOf("GET /5/off_long") >= 0) {
              //Serial.println("GPIO 5 OFF (Long press)");
              output5State = "off"; digitalWrite(output5, LOW); output5LongPressAction = "No action"; actionTaken = true;
            }
            // GPIO 4 - Standard Short Press Toggle
            else if (header.indexOf("GET /4/on") >= 0 && header.indexOf("GET /4/on_long") < 0 && header.indexOf("GET /4/hold_on") < 0) {
              //Serial.println("GPIO 4 ON (Short press)");
              playSound(1);
              output4State = "on"; digitalWrite(output4, HIGH); output4LongPressAction = "Playing sound"; actionTaken = true;
            } else if (header.indexOf("GET /4/off") >= 0 && header.indexOf("GET /4/off_long") < 0 && header.indexOf("GET /4/hold_off") < 0) {
              //Serial.println("GPIO 4 OFF (Short press)");
              output4State = "off"; digitalWrite(output4, LOW); output4LongPressAction = "Stopped"; actionTaken = true;
            }
            // GPIO 4 - New Hold-to-Activate Logic
            else if (header.indexOf("GET /4/hold_on") >= 0) {
              //Serial.println("GPIO 4 Hold ON");
              output4State = "on"; digitalWrite(output4, HIGH); output4LongPressAction = "Powering off system"; actionTaken = true;
              powerOff();
            } else if (header.indexOf("GET /4/hold_off ") >= 0) {
              //Serial.println("GPIO 4 Hold OFF (Released)");
              output4State = "off"; digitalWrite(output4, LOW); output4LongPressAction = "released hold"; actionTaken = true;
            }
            // --- Handle Directional Controls (with Hold functionality) ---
            // Forward
            else if (header.indexOf("GET /dpad/forward_hold_on") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed; // Update global speed
              //Serial.println(String("Direction: Holding Forward at speed ") + requestedSpeed);
              driveWheels(requestedSpeed, requestedSpeed);
              lastDirectionalAction = "Holding Forward (" + String(requestedSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/forward_hold_off") >= 0) {
              //Serial.println("Direction: Released Forward");
              driveStop();
              lastDirectionalAction = "Released Forward (was " + String(currentSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/forward_long") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Long Forward (Generic) at speed ") + requestedSpeed);
              lastDirectionalAction = "Long Forward (" + String(requestedSpeed) + ")"; actionTaken = true;
              driveWheels(requestedSpeed, requestedSpeed);
            } else if (header.indexOf("GET /dpad/forward") >= 0) { // Short press
              //Serial.println("Direction: Forward (Short)");
              // For short press, current behavior is stop. If you want a pulse:
              // requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              // driveWheels(requestedSpeed, requestedSpeed); delay(100); driveStop();
              driveStop();
              currentSpeed = requestedSpeed;
              lastDirectionalAction = "Forward (Pulse/Stop)"; actionTaken = true;
            }
            // Left
            else if (header.indexOf("GET /dpad/left_hold_on") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Holding Left at speed ") + requestedSpeed);
              driveLeft(requestedSpeed);
              lastDirectionalAction = "Holding Left (" + String(requestedSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/left_hold_off") >= 0) {
              //Serial.println("Direction: Released Left");
              driveStop();
              lastDirectionalAction = "Released Left (was " + String(currentSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/left_long") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Long Left (Generic) at speed ") + requestedSpeed);
              lastDirectionalAction = "Long Left (" + String(requestedSpeed) + ")"; actionTaken = true;
              driveLeft(requestedSpeed);
            } else if (header.indexOf("GET /dpad/left") >= 0) {
              //Serial.println("Direction: Left (Short)");
              driveStop();
              lastDirectionalAction = "Left (Pulse/Stop)"; actionTaken = true;
            }
            // Right
            else if (header.indexOf("GET /dpad/right_hold_on") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Holding Right at speed ") + requestedSpeed);
              driveRight(requestedSpeed);
              lastDirectionalAction = "Holding Right (" + String(requestedSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/right_hold_off") >= 0) {
              //Serial.println("Direction: Released Right");
              driveStop();
              lastDirectionalAction = "Released Right (was " + String(currentSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/right_long") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Long Right (Generic) at speed ") + requestedSpeed);
              driveRight(requestedSpeed);
              lastDirectionalAction = "Long Right (" + String(requestedSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/right") >= 0) {
              //Serial.println("Direction: Right (Short)");
              driveStop();
              lastDirectionalAction = "Right (Pulse/Stop)"; actionTaken = true;
            }
            // Down (Backward)
            else if (header.indexOf("GET /dpad/down_hold_on") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Holding Down at speed ") + requestedSpeed);
              driveWheels(-requestedSpeed, -requestedSpeed); // Negative for backward
              lastDirectionalAction = "Holding Down (" + String(requestedSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/down_hold_off") >= 0) {
              //Serial.println("Direction: Released Down");
              driveStop();
              lastDirectionalAction = "Released Down (was " + String(currentSpeed) + ")"; actionTaken = true;
            } else if (header.indexOf("GET /dpad/down_long") >= 0) {
              requestedSpeed = parseSpeedFromHeader(header, currentSpeed);
              currentSpeed = requestedSpeed;
              //Serial.println(String("Direction: Long Down (Generic) at speed ") + requestedSpeed);
              lastDirectionalAction = "Long Down (" + String(requestedSpeed) + ")"; actionTaken = true;
              driveWheels(-requestedSpeed, -requestedSpeed);
            } else if (header.indexOf("GET /dpad/down") >= 0) {
              //Serial.println("Direction: Down (Short)");
              driveStop();
              lastDirectionalAction = "Down (Pulse/Stop)"; actionTaken = true;
            }


            if (actionTaken) {
              // Send JSON response for AJAX requests
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close");
              client.println("Access-Control-Allow-Origin: *"); // Good for development
              client.println();

              String jsonResponse = "{";
              jsonResponse += "\"output5State\":\"" + output5State + "\",";
              jsonResponse += "\"output5LongPressAction\":\"" + output5LongPressAction + "\",";
              jsonResponse += "\"output4State\":\"" + output4State + "\",";
              jsonResponse += "\"output4LongPressAction\":\"" + output4LongPressAction + "\",";
              jsonResponse += "\"currentSpeed\":" + String(currentSpeed) + ",";
              jsonResponse += "\"lastDirectionalAction\":\"" + lastDirectionalAction + "\"";
              jsonResponse += "}";
              client.print(jsonResponse);
              //Serial.println("Sent JSON response: " + jsonResponse);

            }
            else {
            if (header.indexOf("GET /status ") >= 0) { // If no action was taken, check for /status
            //Serial.println("GET /status received, sending JSON status.")
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println("Access-Control-Allow-Origin: *");
            client.println();

            String jsonResponse = "{";
            jsonResponse += "\"output5State\":\"" + output5State + "\",";
            jsonResponse += "\"output5LongPressAction\":\"" + output5LongPressAction + "\",";
            jsonResponse += "\"output4State\":\"" + output4State + "\",";
            jsonResponse += "\"output4LongPressAction\":\"" + output4LongPressAction + "\",";
            jsonResponse += "\"currentSpeed\":" + String(currentSpeed) + ",";
            jsonResponse += "\"lastDirectionalAction\":\"" + lastDirectionalAction + "\"";
            jsonResponse += "}";
            client.print(jsonResponse);
            //Serial.println("Sent JSON status for /status: " + jsonResponse);
            } 
             else if (header.indexOf("GET / ") >= 0 || header.indexOf("GET /index.html") >= 0) {
              // Serve the full HTML page
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<title>Roomba Robot Control</title>");
              client.println("<style>");
              client.println("html { font-family: Helvetica, Arial, sans-serif; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println("body { margin-top: 20px; background-color: #f4f4f4; color: #333; }");
              client.println("h1 { color: #333; margin-bottom: 20px; }");
              client.println(".button { background-color: #195B6A; border: none; color: white; padding: 14px 28px; text-decoration: none; font-size: 18px; margin: 5px 2px; cursor: pointer; border-radius: 8px; transition: background-color 0.3s, opacity 0.2s, transform 0.1s; min-width: 110px; box-shadow: 0 2px 4px rgba(0,0,0,0.2);}");
              client.println(".button:hover { background-color: #144a56; }");
              client.println(".button:active { transform: translateY(1px); }");
              client.println(".button.pressed { opacity: 0.7; background-color: #FFA500 !important; }");
              client.println(".button.holding, .d-pad-button.holding { background-color: #FF8C00 !important; }");
              client.println(".button2 { background-color: #77878A; }");
              client.println(".button2:hover { background-color: #667679; }");

              client.println(".controls-wrapper { display: flex; flex-direction: column; align-items: center; gap: 20px; }"); // Reduced gap a bit

              client.println(".speed-control-container { margin-bottom: 15px; display: flex; align-items: center; justify-content: center; gap: 10px; }");
              client.println(".speed-control-container label { font-size: 1.1em; font-weight: bold; }");
              client.println(".speed-control-container input[type='number'] { padding: 8px; font-size: 1em; width: 70px; border-radius: 5px; border: 1px solid #ccc; text-align: center; }");
              client.println(".speed-control-container input[type='number']:focus { border-color: #195B6A; outline: none; }");
              client.println("#current-speed-display { font-weight: bold; color: #195B6A; }");


              client.println(".directional-pad-container { display: grid; grid-template-columns: auto auto auto; grid-template-rows: auto auto auto; gap: 8px; width: fit-content; margin: 0 auto; justify-items: center; align-items: center; }"); // Removed top margin here, handled by wrapper
              client.println(".d-pad-button-link { text-decoration: none; }");
              client.println(".d-pad-button { background-color: #4CAF50; border: none; color: white; padding: 10px; text-align: center; font-size: 16px; cursor: pointer; border-radius: 8px; min-width: 75px; min-height: 55px; display: flex; flex-direction: column; align-items: center; justify-content: center; transition: background-color 0.3s, box-shadow 0.1s; box-shadow: 0 2px 4px rgba(0,0,0,0.15);}");
              client.println(".d-pad-button .arrow { font-size: 20px; line-height: 1; }");
              client.println(".d-pad-button:hover { background-color: #45a049; }");
              client.println(".d-pad-button:active { transform: translateY(1px); box-shadow: 0 1px 2px rgba(0,0,0,0.2); }");
              client.println(".d-pad-button.pressed { opacity: 0.7; background-color: #FFA500 !important; }");
              client.println("#btn-forward { grid-column: 2; grid-row: 1; }");
              client.println("#btn-left { grid-column: 1; grid-row: 2; }");
              client.println(".d-pad-center { grid-column: 2; grid-row: 2; width: 75px; height: 55px; }");
              client.println("#btn-right { grid-column: 3; grid-row: 2; }");
              client.println("#btn-down { grid-column: 2; grid-row: 3; }");
              client.println("#directional-action-msg { font-size: 1em; color: #2a9fd6; min-height:1.2em; margin-top: 5px; font-weight: bold;}"); // Reduced margin

              client.println(".gpio-container { display: flex; justify-content: space-evenly; align-items: flex-start; margin-top: 20px; margin-bottom: 20px; flex-wrap: wrap; gap: 20px; width: 100%; max-width: 600px;}");
              client.println(".gpio-control { background-color: #fff; border: 1px solid #ddd; border-radius: 10px; padding: 20px; margin: 10px; min-width: 220px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); text-align: center;}");
              client.println(".gpio-control h2 { margin-top: 0; color: #555; }");
              client.println(".gpio-control p { margin: 10px 0; font-size: 1.1em;}");
              client.println(".status-message { font-size: 0.9em; color: #007bff; min-height:1.2em; margin-top: 15px !important;}");
              client.println("</style>");

              client.println("<script>");
              client.println("const longPressDuration = 200;"); // milliseconds
              client.println("let pressTimer = null;");
              client.println("let isLongPressGlobal = false;");
              client.println("let isGPIO4Holding = false;");
              client.println("let dpadHoldingState = { forward: false, left: false, right: false, down: false };");

              client.println("function updateUI(data) {");
              client.println("  if (!data) return;");
              // Update GPIO 5
              client.println("  const gpio5StateEl = document.getElementById('gpio5-state');");
              client.println("  const gpio5ActionMsgEl = document.getElementById('gpio5-action-msg');");
              client.println("  const gpio5Link = document.getElementById('gpio5-btn-link');");
              client.println("  const gpio5Button = gpio5Link ? gpio5Link.querySelector('button') : null;");
              client.println("  if(gpio5StateEl) gpio5StateEl.textContent = data.output5State;");
              client.println("  if(gpio5ActionMsgEl) gpio5ActionMsgEl.textContent = data.output5LongPressAction && data.output5LongPressAction !== 'none' ? 'Last GPIO5: ' + data.output5LongPressAction : '';");
              client.println("  if (gpio5Button && gpio5Link) { if (data.output5State === 'on') { gpio5Button.textContent = 'TURN OFF'; gpio5Button.classList.add('button2'); gpio5Link.dataset.action = 'off'; } else { gpio5Button.textContent = 'TURN ON'; gpio5Button.classList.remove('button2'); gpio5Link.dataset.action = 'on'; }}");

              // Update GPIO 4
              client.println("  const gpio4StateEl = document.getElementById('gpio4-state');");
              client.println("  const gpio4ActionMsgEl = document.getElementById('gpio4-action-msg');");
              client.println("  const gpio4Link = document.getElementById('gpio4-btn-link');");
              client.println("  const gpio4Button = gpio4Link ? gpio4Link.querySelector('button') : null;");
              client.println("  if(gpio4StateEl) gpio4StateEl.textContent = data.output4State;");
              client.println("  if(gpio4ActionMsgEl) gpio4ActionMsgEl.textContent = data.output4LongPressAction && data.output4LongPressAction !== 'none' ? 'Last GPIO4: ' + data.output4LongPressAction : '';");
              client.println("  if (gpio4Button && gpio4Link) {");
              client.println("    if (data.output4State === 'on') { gpio4Button.textContent = 'TURN OFF'; gpio4Button.classList.add('button2'); gpio4Link.dataset.action = 'off'; }");
              client.println("    else { gpio4Button.textContent = 'TURN ON'; gpio4Button.classList.remove('button2'); gpio4Link.dataset.action = 'on'; }");
              client.println("    if (data.output4LongPressAction === 'Holding ON') { gpio4Button.classList.add('holding'); } else { gpio4Button.classList.remove('holding'); }");
              client.println("  }");

              // Update Current Speed Display ----
              client.println("  const speedDisplayEl = document.getElementById('current-speed-val');");
              client.println("  if(speedDisplayEl && data.currentSpeed !== undefined) speedDisplayEl.textContent = data.currentSpeed;");
              client.println("  const speedInputEl = document.getElementById('speedControlInput');"); // Update input field as well to reflect server's currentSpeed
              client.println("  if(speedInputEl && data.currentSpeed !== undefined && document.activeElement !== speedInputEl) { speedInputEl.value = data.currentSpeed; }");


              // Update Directional Action Message & D-pad button visual state
              client.println("  const dirActionMsgEl = document.getElementById('directional-action-msg');");
              client.println("  if(dirActionMsgEl && data.lastDirectionalAction && data.lastDirectionalAction !== 'none') { dirActionMsgEl.textContent = 'Direction: ' + data.lastDirectionalAction; } else if (dirActionMsgEl) { /* dirActionMsgEl.textContent = ''; */ }");
              client.println("  document.querySelectorAll('.d-pad-button').forEach(btn => { btn.classList.remove('holding'); });"); // Clear holding class
              client.println("  if (data.lastDirectionalAction) {"); // Apply holding class if applicable
              client.println("    if (data.lastDirectionalAction.startsWith('Holding')) {");
              client.println("      const dir = data.lastDirectionalAction.split(' ')[1].toLowerCase();");
              client.println("      const heldButtonLink = document.querySelector('#btn-' + dir);"); // Get the link
              client.println("      if (heldButtonLink) { const heldButton = heldButtonLink.querySelector('button'); if(heldButton) heldButton.classList.add('holding'); }");
              client.println("    }");
              client.println("  }");
              client.println("}");

              client.println("function sendRequestAndUpdateUI(url) {");
              client.println("  fetch(url)");
              client.println("    .then(response => { if (!response.ok) { throw new Error('Network response was not ok: ' + response.statusText + ' (' + response.status + ')'); } return response.json(); })");
              client.println("    .then(data => { console.log('Received data:', data); updateUI(data); })");
              client.println("    .catch(error => { console.error('Error:', error); document.getElementById('error-message-area').textContent = 'Error: ' + error.message; });");
              client.println("}");

              client.println("function handlePress(event, linkElement, controlGroup, action) {");
              client.println("  event.preventDefault(); isLongPressGlobal = false;");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  if(button) button.classList.add('pressed');");

              client.println("  pressTimer = setTimeout(() => {");
              client.println("    isLongPressGlobal = true;");
              // Get speed for D-Pad commands ----
              client.println("    let speedQueryParam = '';");
              client.println("    if (controlGroup === 'dpad') {");
              client.println("      const speedInput = document.getElementById('speedControlInput');");
              client.println("      if (speedInput && speedInput.value) { speedQueryParam = '?speed=' + speedInput.value; }");
              client.println("    }");

              client.println("    if (controlGroup === '4') {");
              client.println("      isGPIO4Holding = true;");
              client.println("      if(button) button.classList.add('holding');");
              client.println("      console.log('GPIO4 Hold ON: /4/hold_on');");
              client.println("      sendRequestAndUpdateUI('/4/hold_on');");
              client.println("    } else if (controlGroup === 'dpad') {");
              client.println("      dpadHoldingState[action] = true;");
              client.println("      if(button) button.classList.add('holding');");
              client.println("      console.log('DPAD Hold ON: /dpad/' + action + '_hold_on' + speedQueryParam);");
              client.println("      sendRequestAndUpdateUI('/dpad/' + action + '_hold_on' + speedQueryParam);");
              client.println("    } else {"); // Generic long press for other controls (e.g., GPIO5)
              client.println("      console.log('Long press: /' + controlGroup + '/' + action + '_long');");
              client.println("      sendRequestAndUpdateUI('/' + controlGroup + '/' + action + '_long');");
              client.println("    }");
              client.println("  }, longPressDuration);");
              client.println("}");

              client.println("function handleRelease(event, linkElement, controlGroup, action) {");
              client.println("  clearTimeout(pressTimer);");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  if(button) { button.classList.remove('pressed'); button.classList.remove('holding'); }");

              // Get speed for D-Pad short press (if applicable, though current server logic stops) ----
              client.println("  let speedQueryParam = '';");
              client.println("  if (controlGroup === 'dpad' && !isLongPressGlobal) {"); // Only for short press if needed
              client.println("      const speedInput = document.getElementById('speedControlInput');");
              client.println("      if (speedInput && speedInput.value) { speedQueryParam = '?speed=' + speedInput.value; }");
              client.println("  }");


              client.println("  if (controlGroup === '4' && isGPIO4Holding) {");
              client.println("    console.log('GPIO4 Hold OFF (Release): /4/hold_off');");
              client.println("    sendRequestAndUpdateUI('/4/hold_off');");
              client.println("    isGPIO4Holding = false; isLongPressGlobal = false; return;");
              client.println("  } else if (controlGroup === 'dpad' && dpadHoldingState[action]) {");
              client.println("    console.log('DPAD Hold OFF (Release): /dpad/' + action + '_hold_off');");
              // Speed param not usually sent on _hold_off as it's a stop, but server knows currentSpeed
              client.println("    sendRequestAndUpdateUI('/dpad/' + action + '_hold_off');");
              client.println("    dpadHoldingState[action] = false; isLongPressGlobal = false; return;");
              client.println("  }");

              client.println("  if (!isLongPressGlobal) {"); // Short press
              client.println("    console.log('Short press: /' + controlGroup + '/' + action + speedQueryParam);");
              client.println("    sendRequestAndUpdateUI('/' + controlGroup + '/' + action + speedQueryParam);");
              client.println("  } else {");
              client.println("    console.log('Generic long press release for ' + controlGroup + ' (action sent on press)');");
              client.println("  }");
              client.println("  isLongPressGlobal = false;");
              client.println("}");

              client.println("function handleCancel(linkElement) {");
              client.println("  clearTimeout(pressTimer);");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  const controlGroup = linkElement.dataset.gpio || linkElement.dataset.controlGroup;");
              client.println("  const action = linkElement.dataset.action;");
              client.println("  if(button) { button.classList.remove('pressed'); button.classList.remove('holding'); }");

              client.println("  if (controlGroup === '4' && isGPIO4Holding) {");
              client.println("    console.log('GPIO4 Hold OFF (Cancel): /4/hold_off'); sendRequestAndUpdateUI('/4/hold_off'); isGPIO4Holding = false;");
              client.println("  } else if (controlGroup === 'dpad' && dpadHoldingState[action]) {");
              client.println("    console.log('DPAD Hold OFF (Cancel): /dpad/' + action + '_hold_off'); sendRequestAndUpdateUI('/dpad/' + action + '_hold_off'); dpadHoldingState[action] = false;");
              client.println("  }");
              client.println("  isLongPressGlobal = false;");
              client.println("}");

              client.println("document.addEventListener('DOMContentLoaded', () => {");
              client.println("  sendRequestAndUpdateUI('/status');"); 
              client.println("  document.querySelectorAll('.long-press-btn').forEach(link => {"); // GPIO buttons
              client.println("    const button = link.querySelector('button'); const controlGroup = link.dataset.gpio;");
              client.println("    if (button) {");
              client.println("      button.addEventListener('mousedown', (e) => handlePress(e, link, controlGroup, link.dataset.action));");
              client.println("      button.addEventListener('mouseup', (e) => handleRelease(e, link, controlGroup, link.dataset.action));");
              client.println("      button.addEventListener('mouseleave', (e) => handleCancel(link));"); // Pass link
              client.println("      button.addEventListener('touchstart', (e) => handlePress(e, link, controlGroup, link.dataset.action), { passive: false });");
              client.println("      button.addEventListener('touchend', (e) => handleRelease(e, link, controlGroup, link.dataset.action));");
              client.println("      button.addEventListener('touchcancel', (e) => handleCancel(link));"); // Pass link
              client.println("    }");
              client.println("  });");
              client.println("  document.querySelectorAll('.d-pad-button-link').forEach(link => {"); // D-Pad buttons
              client.println("    const button = link.querySelector('button'); const controlGroup = link.dataset.controlGroup; const action = link.dataset.action;");
              client.println("    if (button) {");
              client.println("      button.addEventListener('mousedown', (e) => handlePress(e, link, controlGroup, action));");
              client.println("      button.addEventListener('mouseup', (e) => handleRelease(e, link, controlGroup, action));");
              client.println("      button.addEventListener('mouseleave', (e) => handleCancel(link));"); // Pass link
              client.println("      button.addEventListener('touchstart', (e) => handlePress(e, link, controlGroup, action), { passive: false });");
              client.println("      button.addEventListener('touchend', (e) => handleRelease(e, link, controlGroup, action));");
              client.println("      button.addEventListener('touchcancel', (e) => handleCancel(link));"); // Pass link
              client.println("    }");
              client.println("  });");

                // ---- NEW: Event listener for speed input to potentially update server or just use client-side ----
                // For now, speed is read on D-Pad press. If you want to "set" speed on ESP32 when input changes:
                
              client.println("  const speedInput = document.getElementById('speedControlInput');");
              client.println("  if (speedInput) {");
              client.println("    speedInput.addEventListener('change', (event) => {");
              client.println("      const newSpeed = event.target.value;");
              //client.println("      // Optionally send to server to update its 'currentSpeed' default immediately");
              //client.println("      // sendRequestAndUpdateUI('/setSpeed?value=' + newSpeed);
              client.println("      console.log('Speed input changed to: ' + newSpeed);");
              client.println("      document.getElementById('current-speed-val').textContent = newSpeed;");
              client.println("    });");
              client.println("  }");
              

              client.println("});");
              client.println("</script>");
              client.println("</head>");

              client.println("<body><h1>ESP32 Advanced Control</h1>");
              client.println("<div id=\"error-message-area\" style=\"color: red; margin-bottom: 10px;\"></div>");
              client.println("<div class=\"controls-wrapper\">");

              // ---- NEW: Speed Control Input Field ----
              client.println("<div class=\"speed-control-container\">");
              client.println("  <label for=\"speedControlInput\">Speed (0-500): </label>");
              client.println(String("  <input type=\"number\" id=\"speedControlInput\" name=\"speed\" min=\"0\" max=\"500\" value=\"") + String(currentSpeed) + "\">");
              client.println("  <span>Current Set: <span id=\"current-speed-val\">" + String(currentSpeed) + "</span></span>");
              client.println("</div>");

              client.println("<div>"); // D-pad section wrapper
              client.println("  <div class=\"directional-pad-container\">");
              client.println("    <a href=\"#\" id=\"btn-forward\" class=\"d-pad-button-link\" data-control-group=\"dpad\" data-action=\"forward\"><button class=\"d-pad-button\"><span class=\"arrow\">▲</span>Forward</button></a>");
              client.println("    <a href=\"#\" id=\"btn-left\" class=\"d-pad-button-link\" data-control-group=\"dpad\" data-action=\"left\"><button class=\"d-pad-button\"><span class=\"arrow\">◄</span>Left</button></a>");
              client.println("    <div class=\"d-pad-center\"></div>"); // Placeholder for center
              client.println("    <a href=\"#\" id=\"btn-right\" class=\"d-pad-button-link\" data-control-group=\"dpad\" data-action=\"right\"><button class=\"d-pad-button\">Right<span class=\"arrow\">►</span></button></a>");
              client.println("    <a href=\"#\" id=\"btn-down\" class=\"d-pad-button-link\" data-control-group=\"dpad\" data-action=\"down\"><button class=\"d-pad-button\"><span class=\"arrow\">▼</span>Down</button></a>");
              client.println("  </div>");
              client.println("  <p id=\"directional-action-msg\">" + (lastDirectionalAction != "none" ? "Direction: " + lastDirectionalAction : "No direction yet") + "</p>");
              client.println("</div>");

              client.println("<div class=\"gpio-container\">");
              // GPIO 5 Control Block
              client.println("<div class=\"gpio-control\">");
              client.println("<h2>Start Stop</h2>");
              client.println("<p>State: <strong id=\"gpio5-state\">" + output5State + "</strong></p>");
              client.println(String("<p class=\"status-message\" id=\"gpio5-action-msg\">") + (output5LongPressAction != "none" ? "Last GPIO5: " + output5LongPressAction : "") + "</p>");
              client.print(String("<p><a id=\"gpio5-btn-link\" href=\"#\" class=\"long-press-btn\" data-gpio=\"5\" data-action=\"") + (output5State == "off" ? "on" : "off") + "\">");
              client.print( (String("<button class=\"button ") + (output5State == "on" ? "button2" : "") + "\">") + String( (output5State == "off" ? "TURN ON" : "TURN OFF") ) + String("</button></a></p>") );
              client.println("</div>");
              // GPIO 4 Control Block
              client.println("<div class=\"gpio-control\">");
              client.println("<h2>Power off</h2>");
              client.println("<p>State: <strong id=\"gpio4-state\">" + output4State + "</strong></p>");
              client.println(String("<p class=\"status-message\" id=\"gpio4-action-msg\">") + (output4LongPressAction != "none" ? "Last GPIO4: " + output4LongPressAction : "") + "</p>");
              client.print(String("<p><a id=\"gpio4-btn-link\" href=\"#\" class=\"long-press-btn\" data-gpio=\"4\" data-action=\"") + (output4State == "off" ? "on" : "off") + "\">");
              client.print( (String("<button class=\"button ") + (output4State == "on" ? "button2" : "") + "\">") + String( (output4State == "off" ? "TURN ON" : "TURN OFF") ) + String("</button></a></p>") );
              client.println("</div>");
              client.println("</div>"); // end gpio-container

              client.println("</div>"); // end controls-wrapper
              client.println("</body></html>");

            } else { // If no specific action URL was matched for JSON and not root path for HTML
              //Serial.println("Sending 404 Not Found");
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-type:text/plain"); client.println("Connection: close"); client.println();
              client.println("Not Found");
            }
            }
            break; // Exit the while client.available loop once request is processed
          } else { currentLine = ""; } // if c == '\n' but currentLine is not empty, it's a new line in header
        } else if (c != '\r') { currentLine += c; } // Add character to currentLine
      } // end if client.available
      if (!client.connected() && (currentTime - previousTime > timeoutTime)) {
        //Serial.println("Client connection timed out or closed by client before full header.");
        break;
      }
    } // end while client.connected
    // Close the connection
    client.stop();
    //Serial.println("Client disconnected.");
    //Serial.println("");
  } // end if client
}