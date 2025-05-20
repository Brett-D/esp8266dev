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
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    state = toggle(state);
    digitalWrite(output4, state);
    Serial.print(".");
    Serial.println("PWSTANDBY");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

void loop() {
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) { // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    header = ""; // Clear header for new request

    // loop while the client's connected and the timeout has not been reached
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) { // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        // Serial.write(c); // Optional: print raw request to serial
        header += c;
        if (c == '\n') { // if the byte is a newline character
          if (currentLine.length() == 0) { // End of HTTP request headers
            Serial.println("Request Header Received:");
            Serial.println(header);

            bool actionTaken = false; // Flag to check if a GPIO action was processed

            // --- Handle GPIO state changes based on request ---
            // GPIO 5 Short Press
            if (header.indexOf("GET /5/on ") >= 0 && header.indexOf("GET /5/on_long ") < 0) {
              Serial.println("GPIO 5 ON (Short press)");
              output5State = "on";
              digitalWrite(output5, HIGH);
              output5LongPressAction = "Short ON";
              actionTaken = true;
            } else if (header.indexOf("GET /5/off ") >= 0 && header.indexOf("GET /5/off_long ") < 0) {
              Serial.println("GPIO 5 OFF (Short press)");
              output5State = "off";
              digitalWrite(output5, LOW);
              output5LongPressAction = "Short OFF";
              actionTaken = true;
            }
            // GPIO 5 Long Press
            else if (header.indexOf("GET /5/on_long ") >= 0) {
              Serial.println("GPIO 5 ON (Long press)");
              output5State = "on"; 
              digitalWrite(output5, HIGH); 
              output5LongPressAction = "Long ON Activated";
              actionTaken = true;
            } else if (header.indexOf("GET /5/off_long ") >= 0) {
              Serial.println("GPIO 5 OFF (Long press)");
              output5State = "off";
              digitalWrite(output5, LOW);
              output5LongPressAction = "Long OFF Activated";
              actionTaken = true;
            }
            // GPIO 4 Short Press
            else if (header.indexOf("GET /4/on ") >= 0 && header.indexOf("GET /4/on_long ") < 0) {
              Serial.println("GPIO 4 ON (Short press)");
              output4State = "on";
              digitalWrite(output4, HIGH);
              output4LongPressAction = "Short ON";
              actionTaken = true;
            } else if (header.indexOf("GET /4/off ") >= 0 && header.indexOf("GET /4/off_long ") < 0) {
              Serial.println("GPIO 4 OFF (Short press)");
              output4State = "off";
              digitalWrite(output4, LOW);
              output4LongPressAction = "Short OFF";
              actionTaken = true;
            }
            // GPIO 4 Long Press
            else if (header.indexOf("GET /4/on_long ") >= 0) {
              Serial.println("GPIO 4 ON (Long press)");
              output4State = "on";
              digitalWrite(output4, HIGH);
              output4LongPressAction = "Long ON Activated";
              actionTaken = true;
            } else if (header.indexOf("GET /4/off_long ") >= 0) {
              Serial.println("GPIO 4 OFF (Long press)");
              output4State = "off";
              digitalWrite(output4, LOW);
              output4LongPressAction = "Long OFF Activated";
              actionTaken = true;
            }

            if (actionTaken) {
              // Send JSON response for AJAX requests
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close");
              client.println("Access-Control-Allow-Origin: *"); // Optional: For CORS if client is on different origin
              client.println(); // Blank line before body

              String jsonResponse = "{";
              jsonResponse += "\"output5State\":\"" + output5State + "\",";
              jsonResponse += "\"output5LongPressAction\":\"" + output5LongPressAction + "\",";
              jsonResponse += "\"output4State\":\"" + output4State + "\",";
              jsonResponse += "\"output4LongPressAction\":\"" + output4LongPressAction + "\"";
              jsonResponse += "}";
              client.print(jsonResponse);
              Serial.println("Sent JSON response: " + jsonResponse);

            } else if (header.indexOf("GET / ") >= 0 || header.indexOf("GET /index.html") >=0 ) {
              // Serve the full HTML page for root requests
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // --- Display the HTML web page ---
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<title>ESP32 GPIO Control</title>");
              // CSS
              client.println("<style>");
              client.println("html { font-family: Helvetica, Arial, sans-serif; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println("body { margin-top: 20px; background-color: #f4f4f4; color: #333; }");
              client.println("h1 { color: #333; }");
              client.println(".button { background-color: #195B6A; border: none; color: white; padding: 14px 30px; text-decoration: none; font-size: 20px; margin: 5px 2px; cursor: pointer; border-radius: 8px; transition: background-color 0.3s, opacity 0.2s, transform 0.1s; min-width: 120px; box-shadow: 0 2px 4px rgba(0,0,0,0.2);}");
              client.println(".button:hover { background-color: #144a56; }");
              client.println(".button:active { transform: translateY(1px); }"); // Click feedback
              client.println(".button.pressed { opacity: 0.7; background-color: #FFA500 !important; }"); // Style for during press / long press indication
              client.println(".button2 { background-color: #77878A; }");
              client.println(".button2:hover { background-color: #667679; }");
              client.println(".gpio-container { display: flex; justify-content: space-evenly; align-items: flex-start; margin-bottom: 20px; flex-wrap: wrap; gap: 20px;}");
              client.println(".gpio-control { background-color: #fff; border: 1px solid #ddd; border-radius: 10px; padding: 20px; margin: 10px; min-width: 240px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); text-align: center;}");
              client.println(".gpio-control h2 { margin-top: 0; color: #555; }");
              client.println(".gpio-control p { margin: 10px 0; font-size: 1.1em;}");
              client.println(".status-message { font-size: 0.9em; color: #007bff; min-height:1.2em; margin-top: 15px !important;}");
              client.println("</style>");

              // JavaScript for Long Press and AJAX Functionality
              client.println("<script>");
              client.println("const longPressDuration = 1000;");
              client.println("let pressTimer = null;");
              client.println("let isLongPressGlobal = false;");

              client.println("function updateGPIO_UI(data) {");
              client.println("  if (!data) return;");
              // Update GPIO 5
              client.println("  const gpio5StateEl = document.getElementById('gpio5-state');");
              client.println("  const gpio5ActionMsgEl = document.getElementById('gpio5-action-msg');");
              client.println("  const gpio5Link = document.getElementById('gpio5-btn-link');");
              client.println("  const gpio5Button = gpio5Link ? gpio5Link.querySelector('button') : null;");
              client.println("  if(gpio5StateEl) gpio5StateEl.textContent = data.output5State;");
              client.println("  if(gpio5ActionMsgEl) gpio5ActionMsgEl.textContent = data.output5LongPressAction && data.output5LongPressAction !== 'none' ? 'Last Action: ' + data.output5LongPressAction : '';");
              client.println("  if (gpio5Button && gpio5Link) {");
              client.println("    if (data.output5State === 'on') {");
              client.println("      gpio5Button.textContent = 'TURN OFF'; gpio5Button.classList.add('button2'); gpio5Link.dataset.action = 'off';");
              client.println("    } else {");
              client.println("      gpio5Button.textContent = 'TURN ON'; gpio5Button.classList.remove('button2'); gpio5Link.dataset.action = 'on';");
              client.println("    }");
              client.println("  }");

              // Update GPIO 4
              client.println("  const gpio4StateEl = document.getElementById('gpio4-state');");
              client.println("  const gpio4ActionMsgEl = document.getElementById('gpio4-action-msg');");
              client.println("  const gpio4Link = document.getElementById('gpio4-btn-link');");
              client.println("  const gpio4Button = gpio4Link ? gpio4Link.querySelector('button') : null;");
              client.println("  if(gpio4StateEl) gpio4StateEl.textContent = data.output4State;");
              client.println("  if(gpio4ActionMsgEl) gpio4ActionMsgEl.textContent = data.output4LongPressAction && data.output4LongPressAction !== 'none' ? 'Last Action: ' + data.output4LongPressAction : '';");
              client.println("  if (gpio4Button && gpio4Link) {");
              client.println("    if (data.output4State === 'on') {");
              client.println("      gpio4Button.textContent = 'TURN OFF'; gpio4Button.classList.add('button2'); gpio4Link.dataset.action = 'off';");
              client.println("    } else {");
              client.println("      gpio4Button.textContent = 'TURN ON'; gpio4Button.classList.remove('button2'); gpio4Link.dataset.action = 'on';");
              client.println("    }");
              client.println("  }");
              client.println("}");

              client.println("function sendRequestAndUpdateUI(url) {");
              client.println("  fetch(url)");
              client.println("    .then(response => { if (!response.ok) { throw new Error('Network response was not ok: ' + response.status + ' ' + response.statusText); } return response.json(); })");
              client.println("    .then(data => { console.log('Received data:', data); updateGPIO_UI(data); })");
              client.println("    .catch(error => { console.error('Error fetching/updating UI:', error); document.getElementById('error-message-area').textContent = 'Error: Could not update GPIO status. ' + error.message; });");
              client.println("}");

              client.println("function handlePress(event, linkElement, gpio, action) {");
              client.println("  event.preventDefault();");
              client.println("  isLongPressGlobal = false;");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  if(button) button.classList.add('pressed');"); // Visual feedback: press down
              
              client.println("  pressTimer = setTimeout(() => {");
              client.println("    isLongPressGlobal = true;");
              client.println("    if(button) { button.style.backgroundColor = '#FFA500'; }"); // Orange for long press active
              client.println("    console.log('Long press triggered: /' + gpio + '/' + action + '_long');");
              client.println("    sendRequestAndUpdateUI('/' + gpio + '/' + action + '_long');");
              client.println("  }, longPressDuration);");
              client.println("}");

              client.println("function handleRelease(event, linkElement, gpio, action) {");
              client.println("  clearTimeout(pressTimer);");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  if(button) { button.classList.remove('pressed'); button.style.backgroundColor = ''; }"); // Reset visual styles
              client.println("  if (!isLongPressGlobal) {");
              client.println("    console.log('Short press: /' + gpio + '/' + action);");
              client.println("    sendRequestAndUpdateUI('/' + gpio + '/' + action);");
              client.println("  }");
              client.println("  isLongPressGlobal = false;");
              client.println("}");
              
              client.println("function handleCancel(linkElement) {");
              client.println("  clearTimeout(pressTimer);");
              client.println("  const button = linkElement.querySelector('button');");
              client.println("  if(button) { button.classList.remove('pressed'); button.style.backgroundColor = ''; }");
              client.println("  isLongPressGlobal = false;");
              client.println("}");

              client.println("document.addEventListener('DOMContentLoaded', () => {");
              client.println("  document.querySelectorAll('.long-press-btn').forEach(link => {");
              client.println("    const button = link.querySelector('button');");
              client.println("    const gpio = link.dataset.gpio;");
              // client.println("    let action = link.dataset.action;"); // 'action' is passed to handlers, no need to store here

              client.println("    if (button) {");
              client.println("      button.addEventListener('mousedown', (e) => handlePress(e, link, gpio, link.dataset.action));");
              client.println("      button.addEventListener('mouseup', (e) => handleRelease(e, link, gpio, link.dataset.action));");
              client.println("      button.addEventListener('mouseleave', (e) => handleCancel(link));");
              client.println("      button.addEventListener('touchstart', (e) => handlePress(e, link, gpio, link.dataset.action), { passive: false });");
              client.println("      button.addEventListener('touchend', (e) => handleRelease(e, link, gpio, link.dataset.action));");
              client.println("      button.addEventListener('touchcancel', (e) => handleCancel(link));");
              client.println("    }");
              client.println("  });");
              client.println("});");
              client.println("</script>");
              client.println("</head>");

              // Web Page Body
              client.println("<body><h1>ESP32 Web Server GPIO Control</h1>");
              client.println("<div id=\"error-message-area\" style=\"color: red; margin-bottom: 10px;\"></div>"); // For displaying errors
              client.println("<div class=\"gpio-container\">");

              // GPIO 5 Controls
              client.println("<div class=\"gpio-control\">");
              client.println("<h2>GPIO 5</h2>");
              client.println("<p>Current State: <strong id=\"gpio5-state\">" + output5State + "</strong></p>");
              client.println(String("<p class=\"status-message\" id=\"gpio5-action-msg\">") + (output5LongPressAction != "none" ? "Last Action: " + output5LongPressAction : "") + "</p>");
              client.print(String("<p><a id=\"gpio5-btn-link\" href=\"#\" class=\"long-press-btn\" data-gpio=\"5\" data-action=\"") + (output5State == "off" ? "on" : "off") + "\">");
              client.print(String("<button class=\"button ") + (output5State == "on" ? "button2" : "") + "\">");
              client.print(output5State == "off" ? "TURN ON" : "TURN OFF"); // This line is fine
              client.println("</button></a></p>");
              client.println("</div>");

              // GPIO 4 Controls
              client.println("<div class=\"gpio-control\">");
              client.println("<h2>GPIO 4</h2>");
              client.println("<p>Current State: <strong id=\"gpio4-state\">" + output4State + "</strong></p>");
              client.println(String("<p class=\"status-message\" id=\"gpio4-action-msg\">") + (output4LongPressAction != "none" ? "Last Action: " + output4LongPressAction : "") + "</p>");
              client.print(String("<p><a id=\"gpio4-btn-link\" href=\"#\" class=\"long-press-btn\" data-gpio=\"4\" data-action=\"") + (output4State == "off" ? "on" : "off") + "\">");
              client.print(String("<button class=\"button ") + (output4State == "on" ? "button2" : "") + "\">");
              client.print(output4State == "off" ? "TURN ON" : "TURN OFF"); // This line is fine
              client.println("</button></a></p>");
              client.println("</div>");

              client.println("</div>"); // End gpio-container
              client.println("</body></html>");

            } else {
              // Handle other paths or send a 404 Not Found
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();
              client.println("Not Found");
              Serial.println("Sent 404 Not Found");
            }
            break; // Break out of the while loop for reading client data
          } else { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') { // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      } // end if client.available
       if (!client.connected() && (currentTime - previousTime > timeoutTime)) { // Check for timeout
         Serial.println("Client connection timed out.");
         break;
       }
    } // end while client.connected
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  } // end if(client)
}
