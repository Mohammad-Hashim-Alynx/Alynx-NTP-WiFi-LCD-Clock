#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>


// ========== LCD SETUP ==========  
LiquidCrystal_I2C lcd(0x27, 20, 4);  // I2C address 0x27, 20x4 LCD


// ========== WEB SERVER ==========
WebServer server(80);


// ========== PREFERENCES (STORAGE) ==========
Preferences preferences;


// ========== BUTTON SETUP ==========
const int CONFIG_BUTTON_PIN = 4; // GPIO 4 for config button
bool configButtonPressed = false;
unsigned long lastButtonCheck = 0;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50;


// ========== TIME & SETTINGS ==========
float timezoneOffset = 5.5; // India Standard Time (UTC+5:30) as float
String wifiSSID = "";
String wifiPassword = "";
bool timeFormat24 = true; // true=24 hour, false=12 hour format
bool configMode = false;
bool forceConfigMode = false; // Set to true only by button press


// ========== SECURITY ==========
const String CONFIG_PASSWORD = "CHANGE_ME"; // Your password


// ========== TIME VARIABLES ==========
unsigned long lastNTPUpdate = 0;
const unsigned long NTP_UPDATE_INTERVAL = 3600000; // 1 hour
struct tm timeinfo;


// ========== WIFI RECONNECTION ==========
unsigned long lastWiFiReconnectAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 60000; // 1 minute normal retry
const unsigned long BOOT_RETRY_INTERVAL = 10000; // 10 seconds during first 2 minutes after boot
unsigned long bootTime = 0;
bool initialBootPhase = true;
bool lastWiFiStatus = false;
bool wifiReconnectEnabled = true;
int bootAttemptCount = 0;


// ========== BOOT SEQUENCE STATES ==========
enum BootState {
  BOOT_START,
  BOOT_WIFI_ATTEMPT_1,
  BOOT_WIFI_ATTEMPT_2,
  BOOT_WIFI_ATTEMPT_3,
  BOOT_QUICK_RETRY,
  BOOT_NORMAL
};
BootState currentBootState = BOOT_START;


// ========== BIG DIGIT CUSTOM CHARACTERS ==========
// the 8 arrays that form each segment of the custom numbers
byte bar1[8] = {B11100,B11110,B11110,B11110,B11110,B11110,B11110,B11100};
byte bar2[8] = {B00111,B01111,B01111,B01111,B01111,B01111,B01111,B00111};
byte bar3[8] = {B11111,B11111,B00000,B00000,B00000,B00000,B11111,B11111};
byte bar4[8] = {B11110,B11100,B00000,B00000,B00000,B00000,B11000,B11100};
byte bar5[8] = {B01111,B00111,B00000,B00000,B00000,B00000,B00011,B00111};
byte bar6[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B11111,B11111};
byte bar7[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B00111,B01111};
byte bar8[8] = {B11111,B11111,B00000,B00000,B00000,B00000,B00000,B00000};


// Function prototypes
void handleRoot();
void handleConfigAuth();
void handleConfigPost();
void handleConfig();
void handleSave();
void setupWebServer();
void createCustomChars();
void custom0(int col);
void custom1(int col);
void custom2(int col);
void custom3(int col);
void custom4(int col);
void custom5(int col);
void custom6(int col);
void custom7(int col);
void custom8(int col);
void custom9(int col);
void printBigNumber(int value, int col);
void displayBigTime();
void setupLCD();
void setupStorage();
void loadSettings();
void saveSettings();
void displayCenteredText(int line, String text);
void displayStartupMessage();
bool connectWiFi();
void startConfigMode();
void syncTime();
void handleWiFiReconnection();
void updateBootState();
void displayBootProgress();
void checkConfigButton();
void manualWiFiReconnect();
void exitConfigMode(); // New function to exit config mode


void setup() {
  Serial.begin(115200);
  Serial.println("Power restored - Alynx Clock booting...");
 
  // Setup config button
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
 
  // Record boot time for initial retry logic
  bootTime = millis();
  initialBootPhase = true;
  bootAttemptCount = 0;
  currentBootState = BOOT_START;
  configMode = false; // Start in normal mode
  forceConfigMode = false; // Not forced yet


  // Initialize hardware
  setupLCD();
  setupStorage();
  loadSettings();
 
  // Check if config button is pressed during boot (hold for 3 seconds)
  unsigned long buttonStartTime = millis();
  while (millis() - buttonStartTime < 3000) {
    if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
      Serial.println("Config button pressed during boot");
      forceConfigMode = true;
      break;
    }
    delay(10);
  }
 
  // Display boot sequence immediately
  displayBootProgress();


  // Create custom characters for big digits
  createCustomChars();
 
  // CRITICAL: Configure WiFi for auto-reconnection after power loss
  WiFi.mode(WIFI_STA);            // Set as WiFi station (client)
  WiFi.setAutoReconnect(true);    // Enable auto-reconnect
  WiFi.persistent(true);          // Save WiFi credentials to flash


  if (forceConfigMode) {
    // User forced config mode via button
    Serial.println("User requested config mode via button");
    startConfigMode();
  } else if (wifiSSID == "") {
    // No saved credentials - must enter config mode
    Serial.println("No saved WiFi credentials");
    startConfigMode();
  } else {
    // Normal operation - try to connect to saved WiFi
    Serial.println("Attempting to reconnect to saved WiFi...");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
   
    // Start web server for status page
    setupWebServer();
   
    // Display initial connection status
    lcd.setCursor(0, 3);
    lcd.print("Reconnecting WiFi...");
   
    // First connection attempt
    if (connectWiFi()) {
      syncTime();
    }
  }
}


void loop() {
  server.handleClient();
 
  // Check config button
  checkConfigButton();
 
  // Check and handle WiFi reconnection (only if not in config mode)
  if (!configMode) {
    handleWiFiReconnection();
    displayBigTime();
  }
 
  // Update boot state based on time
  updateBootState();
 
  // Sync time every hour only if WiFi is connected
  if (!configMode && WiFi.status() == WL_CONNECTED && millis() - lastNTPUpdate > NTP_UPDATE_INTERVAL) {
    syncTime();
    lastNTPUpdate = millis();
  }


  delay(50);
}


// ========== EXIT CONFIG MODE FUNCTION ==========
void exitConfigMode() {
  if (configMode) {
    Serial.println("Exiting config mode");
   
    // Exit config mode
    configMode = false;
    forceConfigMode = false;
   
    // Disable AP mode
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
   
    // Try to reconnect to saved WiFi
    if (wifiSSID != "") {
      WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Exiting Config");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting...");
    } else {
      // No credentials - show message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No WiFi Saved");
      lcd.setCursor(0, 1);
      lcd.print("Enter Config Mode");
      lcd.setCursor(0, 2);
      lcd.print("to setup WiFi");
    }
   
    delay(2000);
  }
}


// ========== CONFIG BUTTON HANDLING ==========
void checkConfigButton() {
  unsigned long currentTime = millis();
 
  // Debounce check
  if (currentTime - lastButtonCheck < BUTTON_DEBOUNCE_DELAY) {
    return;
  }
 
  lastButtonCheck = currentTime;
 
  // Check if button is pressed (LOW because of PULLUP)
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    if (!configButtonPressed) {
      configButtonPressed = true;
      unsigned long pressStartTime = millis();
     
      // Wait to see how long button is held
      while (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
        if (millis() - pressStartTime > 3000) { // 3 second hold
          Serial.println("Config button held for 3 seconds");
         
          if (!configMode) {
            // Enter config mode
            Serial.println("Entering config mode via button");
            configMode = true;
            forceConfigMode = true;
           
            // Clear display and show config mode message
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Button Config Mode");
            lcd.setCursor(0, 1);
            lcd.print("Hold 5s to exit");
            delay(2000);
           
            startConfigMode();
          }
          return;
        }
        delay(100);
      }
     
      // Short press - manual reconnect attempt
      if (!configMode && millis() - pressStartTime < 1000) {
        Serial.println("Short button press - Manual reconnect");
        manualWiFiReconnect();
      }
    }
   
    // ========== ADDED EXIT CONFIG MODE VIA BUTTON ==========
    // Check if button is held for 5 seconds to exit config mode
    if (configMode) {
      unsigned long exitPressStart = millis();
     
      // Wait to see if button is held for 5 seconds to exit config mode
      while (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
        if (millis() - exitPressStart > 5000) { // 5 second hold to exit
          exitConfigMode();
          return;
        }
        delay(100);
      }
    }
    // ========== END ADDED EXIT CONFIG MODE VIA BUTTON ==========
   
  } else {
    configButtonPressed = false;
  }
}


void manualWiFiReconnect() {
  if (configMode || wifiSSID == "") return;
 
  Serial.println("Manual WiFi reconnect triggered");
 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Manual Reconnect");
  lcd.setCursor(0, 1);
  lcd.print("Trying WiFi...");
  lcd.setCursor(0, 2);
  lcd.print("SSID: ");
  lcd.print(wifiSSID.substring(0, 14));
 
  WiFi.disconnect(false);
  delay(1000);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
 
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    lcd.setCursor(0, 3);
    lcd.print("Connecting");
    for (int i = 0; i < (millis() / 500) % 4; i++) {
      lcd.print(".");
    }
    lcd.print("      ");
  }
 
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 3);
    lcd.print("Connected!    ");
    syncTime();
    delay(2000);
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Failed        ");
    delay(2000);
  }
}


// ========== BOOT SEQUENCE FUNCTIONS ==========
void updateBootState() {
  unsigned long currentTime = millis();
 
  if (currentTime < 10000) { // 0-10 seconds
    currentBootState = BOOT_START;
  } else if (currentTime < 20000) { // 10-20 seconds
    currentBootState = BOOT_WIFI_ATTEMPT_1;
  } else if (currentTime < 30000) { // 20-30 seconds
    currentBootState = BOOT_WIFI_ATTEMPT_2;
  } else if (currentTime < 60000) { // 30-60 seconds
    currentBootState = BOOT_WIFI_ATTEMPT_3;
  } else if (currentTime < 120000) { // 1-2 minutes
    currentBootState = BOOT_QUICK_RETRY;
  } else {
    currentBootState = BOOT_NORMAL;
    initialBootPhase = false;
  }
}


void displayBootProgress() {
  lcd.clear();
 
  switch(currentBootState) {
    case BOOT_START:
      lcd.setCursor(0, 0);
      lcd.print("Booting.....");
      lcd.setCursor(0, 1);
      lcd.print("NTP Time Sync");
      lcd.setCursor(0, 2);
      lcd.print("Web Config Started");
      lcd.setCursor(0, 3);
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("IP: ");
        String ip = WiFi.localIP().toString();
        lcd.print(ip.substring(0, 16));
      } else {
        lcd.print("IP: Starting...");
      }
      break;
     
    case BOOT_WIFI_ATTEMPT_1:
      lcd.setCursor(0, 0);
      lcd.print("Booting.....");
      lcd.setCursor(0, 1);
      lcd.print("WiFi Attempt 1");
      lcd.setCursor(0, 2);
      lcd.print("Router booting...");
      lcd.setCursor(0, 3);
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("IP: ");
        String ip = WiFi.localIP().toString();
        lcd.print(ip.substring(0, 16));
      } else {
        lcd.print("IP: Connecting...");
      }
      break;
     
    case BOOT_WIFI_ATTEMPT_2:
      lcd.setCursor(0, 0);
      lcd.print("Booting.....");
      lcd.setCursor(0, 1);
      lcd.print("WiFi Attempt 2");
      lcd.setCursor(0, 2);
      lcd.print("Router starting...");
      lcd.setCursor(0, 3);
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("IP: ");
        String ip = WiFi.localIP().toString();
        lcd.print(ip.substring(0, 16));
      } else {
        lcd.print("IP: Connecting...");
      }
      break;
     
    case BOOT_WIFI_ATTEMPT_3:
      lcd.setCursor(0, 0);
      lcd.print("Booting.....");
      lcd.setCursor(0, 1);
      lcd.print("WiFi Attempt 3");
      lcd.setCursor(0, 2);
      lcd.print("Router ready soon");
      lcd.setCursor(0, 3);
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("IP: ");
        String ip = WiFi.localIP().toString();
        lcd.print(ip.substring(0, 16));
      } else {
        lcd.print("IP: Connecting...");
      }
      break;
     
    case BOOT_QUICK_RETRY:
      lcd.setCursor(0, 0);
      lcd.print("Booting.....");
      lcd.setCursor(0, 1);
      lcd.print("Quick Retry Mode");
      lcd.setCursor(0, 2);
      lcd.print("Trying every 10s");
      lcd.setCursor(0, 3);
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("IP: ");
        String ip = WiFi.localIP().toString();
        lcd.print(ip.substring(0, 16));
      } else {
        lcd.print("Attempt: ");
        lcd.print(bootAttemptCount);
      }
      break;
     
    case BOOT_NORMAL:
      // Will be handled by normal display functions
      break;
  }
}


// ========== WIFI RECONNECTION FUNCTIONS ==========
void handleWiFiReconnection() {
  // Skip if in config mode or no WiFi credentials
  if (configMode || wifiSSID == "") {
    return;
  }
 
  bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
 
  // Check if WiFi status changed
  if (currentWiFiStatus != lastWiFiStatus) {
    lastWiFiStatus = currentWiFiStatus;
    if (currentWiFiStatus) {
      Serial.println("WiFi connected");
      initialBootPhase = false;
     
      // Show connection success briefly
      if (!configMode) {
        lcd.clear();
        displayCenteredText(0, "Connected!");
        displayCenteredText(1, "Alhamdulillah!");
        displayCenteredText(2, "Syncing time...");
        String ip = WiFi.localIP().toString();
        displayCenteredText(3, "IP: " + ip.substring(0, 16));
        delay(2000);
        syncTime();
      }
    } else {
      Serial.println("WiFi disconnected");
    }
  }
 
  // Check if we should attempt reconnection
  if (!currentWiFiStatus) {
    unsigned long timeSinceLastAttempt = millis() - lastWiFiReconnectAttempt;
    unsigned long currentInterval;
   
    // Determine retry interval based on boot phase
    if (initialBootPhase && (millis() - bootTime < 120000)) { // First 2 minutes after boot
      currentInterval = BOOT_RETRY_INTERVAL; // Retry every 10 seconds
     
      // Update display every attempt during boot phase
      if (timeSinceLastAttempt > currentInterval) {
        bootAttemptCount++;
        Serial.print("Boot retry attempt #");
        Serial.println(bootAttemptCount);
        if (!configMode) {
          displayBootProgress();
        }
      }
    } else {
      currentInterval = WIFI_RECONNECT_INTERVAL; // Normal 1-minute retry
      initialBootPhase = false;
    }
   
    if (timeSinceLastAttempt > currentInterval) {
      Serial.println("Attempting WiFi reconnection...");
      lastWiFiReconnectAttempt = millis();
     
      // Try to reconnect in background
      WiFi.disconnect(false);
      delay(100);
      WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    }
  }
}


// ========== BIG DIGIT FUNCTIONS ==========
void createCustomChars() {
  lcd.createChar(1, bar1);
  lcd.createChar(2, bar2);
  lcd.createChar(3, bar3);
  lcd.createChar(4, bar4);
  lcd.createChar(5, bar5);
  lcd.createChar(6, bar6);
  lcd.createChar(7, bar7);
  lcd.createChar(8, bar8);
}


void custom0(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(8); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(2); lcd.write(6); lcd.write(1);
}


void custom1(int col) {
  lcd.setCursor(col, 0); lcd.write(32); lcd.write(32); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(32); lcd.write(32); lcd.write(1);
}


void custom2(int col) {
  lcd.setCursor(col, 0); lcd.write(5); lcd.write(3); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(2); lcd.write(6); lcd.write(6);
}


void custom3(int col) {
  lcd.setCursor(col, 0); lcd.write(5); lcd.write(3); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(7); lcd.write(6); lcd.write(1);
}


void custom4(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(6); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(32); lcd.write(32); lcd.write(1);
}


void custom5(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(3); lcd.write(4);
  lcd.setCursor(col, 1); lcd.write(7); lcd.write(6); lcd.write(1);
}


void custom6(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(3); lcd.write(4);
  lcd.setCursor(col, 1); lcd.write(2); lcd.write(6); lcd.write(1);
}


void custom7(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(8); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(32); lcd.write(32); lcd.write(1);
}


void custom8(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(3); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(2); lcd.write(6); lcd.write(1);
}


void custom9(int col) {
  lcd.setCursor(col, 0); lcd.write(2); lcd.write(3); lcd.write(1);
  lcd.setCursor(col, 1); lcd.write(7); lcd.write(6); lcd.write(1);
}


void printBigNumber(int value, int col) {
  switch(value) {
    case 0: custom0(col); break;
    case 1: custom1(col); break;
    case 2: custom2(col); break;
    case 3: custom3(col); break;
    case 4: custom4(col); break;
    case 5: custom5(col); break;
    case 6: custom6(col); break;
    case 7: custom7(col); break;
    case 8: custom8(col); break;
    case 9: custom9(col); break;
  }
}


void displayBigTime() {
  static unsigned long lastDisplayUpdate = 0;
  static int lastHour = -1;
  static int lastMinute = -1;
  static int lastSecond = -1;
  static String lastDate = "";
  static String lastWiFiStatusText = "";


  if (millis() - lastDisplayUpdate < 1000) return;


  // Always try to get local time (will use cached time if WiFi is disconnected)
  bool timeUpdated = getLocalTime(&timeinfo);
 
  int hour, minute, second;
  String currentDate;
 
  if (timeUpdated) {
    hour = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    second = timeinfo.tm_sec;
   
    // Get current date in format "Thu 04/12/2025"
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %d/%m/%Y", &timeinfo);
    currentDate = String(dateStr);
  } else {
    // Use last known time if we can't get updated time
    if (lastHour == -1) {
      hour = 12;
      minute = 0;
      second = 0;
      currentDate = "--- --/--/----";
    } else {
      hour = lastHour;
      minute = lastMinute;
      second = lastSecond;
      currentDate = lastDate;
    }
  }
 
  // Get WiFi status text (secret code)
  String currentWiFiStatusText = (WiFi.status() == WL_CONNECTED) ? "Alhamdulillah!" : "Allahu Akbar!";
 
  // During initial boot phase, add a small indicator
  if (initialBootPhase && (millis() - bootTime < 120000)) {
    currentWiFiStatusText = "Allahu Akbar!*"; // * indicates boot phase
  }


  // Check if anything important changed
  bool timeChanged = (hour != lastHour) || (minute != lastMinute);
  bool secondChanged = (second != lastSecond);
  bool dateChanged = (currentDate != lastDate);
  bool wifiStatusChanged = (currentWiFiStatusText != lastWiFiStatusText);


  // Only update if something changed
  if (timeChanged || secondChanged || dateChanged || wifiStatusChanged) {
    // Convert to 12-hour format if needed
    int displayHour = hour;
    String period = "";
   
    if (!timeFormat24) {
      period = (hour >= 12) ? "PM" : "AM";
      displayHour = hour % 12;
      if (displayHour == 0) displayHour = 12;
    }
   
    // Clear and redraw only if time or date changed (not just seconds)
    if (timeChanged || dateChanged || wifiStatusChanged) {
      // Clear all lines
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(0, 1);
      lcd.print("                    ");
      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
     
      // Calculate column positions based on desired padding
      // Desired format: "   1  1:  1  1  PM  " (3 spaces left, 4 spaces right)
      // For a 20-column LCD:
      // - 3 spaces on left = column 3 for first character
      // - Total width including spaces = 20
      // - Right padding = 4 spaces
     
      int hourCol, colonCol, minuteCol, periodCol;
     
      if (displayHour >= 10) {
        // Two-digit hour (e.g., 10, 11, 12)
        // Layout: "  1  0:  1  2  PM  "
        hourCol = 3; // Start after 3 spaces
        colonCol = hourCol + 6; // After two 3-column digits
        minuteCol = colonCol + 1; // After colon
        periodCol = minuteCol + 6; // After two minute digits
      } else {
        // Single-digit hour (e.g., 1, 2, 3, etc.)
        // Layout: "  1:  1  2  PM  "
        hourCol = 3; // Start after 3 spaces
        colonCol = hourCol + 3; // After one 3-column digit
        minuteCol = colonCol + 1; // After colon
        periodCol = minuteCol + 6; // After two minute digits
      }
     
      // Display big time on rows 0 and 1
      // Hour tens digit (only for two-digit hours)
      if (displayHour >= 10) {
        printBigNumber(displayHour / 10, hourCol);
        printBigNumber(displayHour % 10, hourCol + 3);
      } else {
        // Single digit hour - show at calculated position
        printBigNumber(displayHour, hourCol);
      }
     
      // Minute tens digit
      printBigNumber(minute / 10, minuteCol);
     
      // Minute units digit
      printBigNumber(minute % 10, minuteCol + 3);
     
      // Display AM/PM for 12-hour format
      if (!timeFormat24) {
        lcd.setCursor(periodCol, 0);
        lcd.print(period);
        // Add 2 spaces after PM/AM to complete the 4 spaces right padding
        lcd.setCursor(periodCol + 2, 0);
        lcd.print("  "); // Two spaces to complete right padding
      } else {
        // For 24-hour format, still need 4 spaces on right
        // Calculate end position and add spaces
        int endCol;
        if (displayHour >= 10) {
          endCol = minuteCol + 6; // After two minute digits
        } else {
          endCol = minuteCol + 6; // After two minute digits
        }
        lcd.setCursor(endCol, 0);
        lcd.print("    "); // Four spaces for right padding
      }
     
      // Display date on row 2 (centered)
      displayCenteredText(2, currentDate);
     
      // Display WiFi status on row 3 (centered)
      displayCenteredText(3, currentWiFiStatusText);
    }
   
    // Update blinking colon
    if (secondChanged) {
      int colonCol;
     
      if (displayHour >= 10) {
        colonCol = 3 + 6; // After two 3-digit hours (3 + 6 = 9)
      } else {
        colonCol = 3 + 3; // After single 3-digit hour (3 + 3 = 6)
      }
     
      lcd.setCursor(colonCol, 0);
      lcd.print((second % 2 == 0) ? " " : ":");
      lcd.setCursor(colonCol, 1);
      lcd.print((second % 2 == 0) ? " " : ":");
    }
   
    // Update stored values
    lastHour = hour;
    lastMinute = minute;
    lastSecond = second;
    lastDate = currentDate;
    lastWiFiStatusText = currentWiFiStatusText;
  }


  lastDisplayUpdate = millis();
}


// ========== HARDWARE SETUP ==========
void setupLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}


void setupStorage() {
  preferences.begin("clock-config", false);
}


void loadSettings() {
  wifiSSID = preferences.getString("wifi_ssid", "");
  wifiPassword = preferences.getString("wifi_pass", "");
  timezoneOffset = preferences.getFloat("timezone", 5.5);
  timeFormat24 = preferences.getBool("time_format", true);
}


void saveSettings() {
  preferences.putString("wifi_ssid", wifiSSID);
  preferences.putString("wifi_pass", wifiPassword);
  preferences.putFloat("timezone", timezoneOffset);
  preferences.putBool("time_format", timeFormat24);
}


// ========== DISPLAY HELPER FUNCTIONS ==========
void displayCenteredText(int line, String text) {
  lcd.setCursor(0, line);
  lcd.print("                    ");
 
  int length = text.length();
  int position = (20 - length) / 2;
  lcd.setCursor(position, line);
  lcd.print(text);
}


void displayStartupMessage() {
  lcd.clear();
  displayCenteredText(0, "WiFi Clock");
  displayCenteredText(1, "NTP Time Sync");
  displayCenteredText(2, "Web Config Ready");
  if (configMode) {
    displayCenteredText(3, "192.168.4.1");
  } else {
    displayCenteredText(3, "Starting...");
  }
  delay(3000);
}


// ========== WIFI & CONFIG MODE ==========
bool connectWiFi() {
  if (wifiSSID == "") return false;
 
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());


  Serial.print("Connecting to ");
  Serial.println(wifiSSID);


  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }


  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP().toString());
   
    lastWiFiStatus = true;
    lastWiFiReconnectAttempt = millis();
    wifiReconnectEnabled = true;
    return true;
  }
 
  Serial.println("\nWiFi Failed - Will retry");
  wifiReconnectEnabled = true;
  return false;
}


void startConfigMode() {
  configMode = true;
  WiFi.softAP("Alynx_Clock", "CHANGE_ME");


  Serial.println("Config Mode Active");
  Serial.println("AP SSID: Alynx_Clock");
  Serial.println("Password: 12345678");
  Serial.println("IP: 192.168.4.1");


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONFIG MODE");
  lcd.setCursor(0, 1);
  lcd.print("Connect to WiFi:");
  lcd.setCursor(0, 2);
  lcd.print("SSID: Alynx_Clock");
  lcd.setCursor(0, 3);
  lcd.print("IP: 192.168.4.1");


  setupWebServer();
}


// ========== TIME SYNCHRONIZATION ==========
void syncTime() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Syncing time with NTP...");
   
    long tzSeconds = (long)(timezoneOffset * 3600);
    configTime(tzSeconds, 0, "pool.ntp.org", "time.nist.gov");
   
    if (!getLocalTime(&timeinfo, 10000)) {
      Serial.println("Failed to obtain time");
      return;
    }
   
    Serial.println("Time synchronized");
    Serial.print("Local time: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
   
    lastNTPUpdate = millis();
  }
}


// ========== WEB SERVER ==========
void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfigAuth);
  server.on("/config", HTTP_POST, handleConfigPost);
  server.on("/save", HTTP_POST, handleSave);
 
  // ========== ADDED WEB EXIT CONFIG MODE ==========
  server.on("/exitconfig", HTTP_GET, []() {
    if (configMode) {
      exitConfigMode();
     
      String html = "<!DOCTYPE html><html><head><title>Exit Config Mode</title><meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='UTF-8'><style>body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; display: flex; justify-content: center; align-items: center; min-height: 100vh; } .card { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; } .success { color: #155724; background: #d4edda; padding: 15px; border-radius: 5px; margin: 15px 0; } .button { background: #2196F3; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; display: inline-block; margin: 10px; }</style></head><body><div class='card'><div class='success'><h2>Config Mode Exited!</h2></div><p>Returning to normal clock operation.</p><p>Clock will try to reconnect to saved WiFi.</p><div><a class='button' href='/'>Back to Status</a></div></div></body></html>";
     
      server.send(200, "text/html", html);
     
      // Show exit message on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Web Exit Config");
      lcd.setCursor(0, 1);
      lcd.print("Mode...");
      delay(2000);
    } else {
      server.send(200, "text/html", "<html><body><h1>Not in Config Mode</h1><p>Clock is already in normal operation.</p><a href='/'>Back to Status</a></body></html>");
    }
  });
  // ========== END ADDED WEB EXIT CONFIG MODE ==========


  server.begin();
  Serial.println("Web server started");
}


void handleConfigAuth() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>WiFi Clock Configuration - Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .login-box { background: white; padding: 30px; width: 100%; max-width: 400px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    input[type="password"] { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; box-sizing: border-box; }
    button { background: #2196F3; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; width: 100%; font-size: 16px; margin: 10px 0; }
    .error { color: red; margin: 10px 0; text-align: center; }
    a { color: #2196F3; text-decoration: none; display: block; text-align: center; margin: 10px 0; }
  </style>
</head>
<body>
  <div class="login-box">
    <h2 style="text-align: center;">Alynx Clock Config</h2>
    <p style="text-align: center;">Enter password to access configuration:</p>
    <form method="post" action="/config">
      <input type="password" name="password" placeholder="Enter password" required>
      <button type="submit">Access Configuration</button>
    </form>
    )rawliteral";
   
  if (server.hasArg("password")) {
    html += "<div class='error'>Incorrect password. Please try again.</div>";
  }


  html += R"rawliteral(
    <a href="/">Back to Status</a>
    <a href="/exitconfig" style="color: #dc3545;">Exit Config Mode</a>
  </div>
</body>
</html>)rawliteral";


  server.send(200, "text/html", html);
}


void handleConfigPost() {
  if (server.hasArg("password") && server.arg("password") == CONFIG_PASSWORD) {
    handleConfig();
  } else {
    handleConfigAuth();
  }
}


void handleConfig() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Alynx Clock Configuration</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .section { background: #f8f9fa; padding: 15px; margin: 10px 0; border-radius: 5px; }
    input, select { padding: 8px; margin: 5px; width: 200px; }
    button { background: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }
    .radio-group { margin: 10px 0; }
    .radio-group label { margin-right: 15px; }
    .success { background: #d4edda; color: #155724; padding: 10px; border-radius: 5px; margin: 10px 0; }
    .exit-button { background: #dc3545; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; display: inline-block; margin: 10px; }
  </style>
</head>
<body>
  <div class="success">
    <strong>Access Granted</strong> - Configure your WiFi Clock
  </div>


  <h1>Alynx Clock Configuration</h1>
   
  <form action="/save" method="post">
    <input type="hidden" name="auth" value=")rawliteral";
  html += CONFIG_PASSWORD;
  html += R"rawliteral(">
     
    <div class="section">
      <h3>WiFi Settings</h3>
      <input type="text" name="ssid" placeholder="WiFi SSID" value=")rawliteral";
  html += wifiSSID;
  html += R"rawliteral("><br>
      <input type="password" name="password" placeholder="WiFi Password" value=")rawliteral";
  html += wifiPassword;
  html += R"rawliteral(">
    </div>
     
    <div class="section">
      <h3>Timezone (India is UTC+5:30)</h3>
      <select name="timezone">
        <option value="-12" )rawliteral";
  html += (timezoneOffset == -12) ? "selected" : "";
  html += R"rawliteral(>(UTC-12) International Date Line West</option>
        <option value="-5" )rawliteral";
  html += (timezoneOffset == -5) ? "selected" : "";
  html += R"rawliteral(>(UTC-5) Eastern Time</option>
        <option value="-6" )rawliteral";
  html += (timezoneOffset == -6) ? "selected" : "";
  html += R"rawliteral(>(UTC-6) Central Time</option>
        <option value="-7" )rawliteral";
  html += (timezoneOffset == -7) ? "selected" : "";
  html += R"rawliteral(>(UTC-7) Mountain Time</option>
        <option value="-8" )rawliteral";
  html += (timezoneOffset == -8) ? "selected" : "";
  html += R"rawliteral(>(UTC-8) Pacific Time</option>
        <option value="0" )rawliteral";
  html += (timezoneOffset == 0) ? "selected" : "";
  html += R"rawliteral(>(UTC) Greenwich Mean Time</option>
        <option value="1" )rawliteral";
  html += (timezoneOffset == 1) ? "selected" : "";
  html += R"rawliteral(>(UTC+1) Central European Time</option>
        <option value="2" )rawliteral";
  html += (timezoneOffset == 2) ? "selected" : "";
  html += R"rawliteral(>(UTC+2) Eastern European Time</option>
        <option value="5.5" )rawliteral";
  html += (timezoneOffset == 5.5) ? "selected" : "";
  html += R"rawliteral(>(UTC+5:30) India Standard Time</option>
        <option value="8" )rawliteral";
  html += (timezoneOffset == 8) ? "selected" : "";
  html += R"rawliteral(>(UTC+8) China Standard Time</option>
        <option value="9" )rawliteral";
  html += (timezoneOffset == 9) ? "selected" : "";
  html += R"rawliteral(>(UTC+9) Japan Standard Time</option>
        <option value="10" )rawliteral";
  html += (timezoneOffset == 10) ? "selected" : "";
  html += R"rawliteral(>(UTC+10) Eastern Australia</option>
      </select>
    </div>
     
    <div class="section">
      <h3>Time Format</h3>
      <div class="radio-group">
        <label>
          <input type="radio" name="time_format" value="24" )rawliteral";
  html += timeFormat24 ? "checked" : "";
  html += R"rawliteral(>
          24-Hour Format (14:30)
        </label>
        <label>
          <input type="radio" name="time_format" value="12" )rawliteral";
  html += !timeFormat24 ? "checked" : "";
  html += R"rawliteral(>
          12-Hour Format (2:30 PM)
        </label>
      </div>
    </div>
     
    <button type="submit">Save Settings</button>
  </form>
   
  <br>
  <a href="/">Back to Status</a>
  <a class="exit-button" href="/exitconfig">Exit Config Mode</a>
</body>
</html>)rawliteral";


  server.send(200, "text/html", html);
}


void handleSave() {
  if (!server.hasArg("auth") || server.arg("auth") != CONFIG_PASSWORD) {
    server.send(403, "text/html", "<html><body><h1>Access Denied</h1><p>Invalid authentication</p><a href='/config'>Back to Login</a></body></html>");
    return;
  }


  String oldSSID = wifiSSID;
  String oldPassword = wifiPassword;


  if (server.hasArg("ssid")) wifiSSID = server.arg("ssid");
  if (server.hasArg("password")) wifiPassword = server.arg("password");
  if (server.hasArg("timezone")) timezoneOffset = server.arg("timezone").toFloat();
  if (server.hasArg("time_format")) timeFormat24 = (server.arg("time_format") == "24");


  saveSettings();


  String response = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Settings Saved</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .card { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
    .success { color: #155724; background: #d4edda; padding: 15px; border-radius: 5px; margin: 15px 0; }
    .button { background: #2196F3; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; display: inline-block; margin: 10px; }
  </style>
</head>
<body>
  <div class="card">
    <div class="success">
      <h2>Settings Saved Successfully!</h2>
    </div>
    <p>Time format: )rawliteral";
  response += timeFormat24 ? "24-Hour" : "12-Hour";
  response += R"rawliteral(</p>
    <p>WiFi will reconnect automatically if credentials changed.</p>
    <div>
      <a class="button" href="/">Back to Status</a>
      <a class="button" href="/config">Configuration</a>
    </div>
  </div>
</body>
</html>)rawliteral";


  server.send(200, "text/html", response);


  // Exit config mode and restart normal operation
  if (oldSSID != wifiSSID || oldPassword != wifiPassword) {
    delay(1000);
    Serial.println("WiFi credentials changed, reconnecting...");
   
    // Exit config mode
    exitConfigMode();
   
    // Reconnect to WiFi
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  }
}


void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Alynx Clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
    .card { background: white; padding: 20px; margin: 10px 0; border-radius: 10px; }
    .button { background: #4CAF50; color: white; padding: 10px 15px; text-decoration: none; border-radius: 5px; display: inline-block; margin: 5px; }
    .info { background: #e3f2fd; padding: 10px; border-radius: 5px; }
    .format-badge { background: #666; color: white; padding: 2px 8px; border-radius: 10px; font-size: 0.8em; }
    .time-display { font-size: 2.5em; font-weight: bold; text-align: center; margin: 20px 0; font-family: 'Courier New', monospace; }
    .status-online { color: green; }
    .status-offline { color: red; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Alynx Clock - NTP Time Sync</h1>
    <div class="info">
      <strong>WiFi Status:</strong> <span class=")rawliteral";
  html += (WiFi.status() == WL_CONNECTED) ? "status-online" : "status-offline";
  html += R"rawliteral(">)rawliteral";
  html += (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
  html += R"rawliteral(</span><br>
      <strong>Time Format:</strong> <span class="format-badge">)rawliteral";
  html += timeFormat24 ? "24-Hour" : "12-Hour";
  html += R"rawliteral(</span><br>
      <strong>Free Memory:</strong> )rawliteral";
  html += ESP.getFreeHeap();
  html += R"rawliteral( bytes
    </div>
    <div class="time-display" id="currentTime">Loading...</div>
  </div>


  <div class="card">
    <h2>Quick Actions</h2>
    <a class="button" href="/config">Configuration</a>
  </div>


  <script>
    function updateTime() {
      var now = new Date();
      var hours = now.getUTCHours();
      var minutes = now.getUTCMinutes();
      var seconds = now.getUTCSeconds();
     
      // Apply timezone offset (client-side approximation)
      var offsetHours = )rawliteral";
  html += timezoneOffset;
  html += R"rawliteral(;
      hours += Math.floor(offsetHours);
      minutes += Math.round((offsetHours % 1) * 60);
     
      // Normalize time
      if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
      } else if (minutes < 0) {
        hours -= 1;
        minutes += 60;
      }
     
      if (hours >= 24) {
        hours -= 24;
      } else if (hours < 0) {
        hours += 24;
      }
     
      var ampm = hours >= 12 ? 'PM' : 'AM';
     
      // Format based on current setting
      var displayHours = hours;
      if (!)rawliteral";
  html += timeFormat24 ? "false" : "true";
  html += R"rawliteral() {
        displayHours = hours % 12;
        displayHours = displayHours ? displayHours : 12; // 0 should be 12
      }
     
      var timeString =
        String(displayHours).padStart(2, '0') + ':' +
        String(minutes).padStart(2, '0') + ':' +
        String(seconds).padStart(2, '0');
       
      if (!)rawliteral";
  html += timeFormat24 ? "false" : "true";
  html += R"rawliteral() {
        timeString += ' ' + ampm;
      }
     
      document.getElementById('currentTime').textContent = timeString;
    }
    setInterval(updateTime, 1000);
    updateTime();
  </script>
</body>
</html>)rawliteral";


  server.send(200, "text/html", html);
}




