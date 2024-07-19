/*
 * ======================================================================================================================
 * SD.h - SD Card
 * ======================================================================================================================
 */

 // Use Teensy SDIO
#define SD_CONFIG  SdioConfig(FIFO_SDIO)

#define CF_NAME           "CONFIG.TXT"
#define KEY_MAX_LENGTH    30                // Config File Key Length
#define VALUE_MAX_LENGTH  30                // Config File Value Length
#define LINE_MAX_LENGTH   VALUE_MAX_LENGTH+KEY_MAX_LENGTH+3   // =, CR, LF 

/* 
 *=======================================================================================================================
 * SD_initialize()
 *=======================================================================================================================
 */
void SD_initialize() {
  if (!SD.begin(SD_ChipSelect)) {
    Output ("SD:NF");
    SystemStatusBits |= SSB_SD;
    delay (5000);
  }
  else {
    SD_exists = true;
    if (!SD.exists(SD_obsdir)) {
      if (SD.mkdir(SD_obsdir)) {
        Output ("SD:MKDIR OBS OK");
        Output ("SD:Online");
        SD_exists = true;
      }
      else {
        Output ("SD:MKDIR OBS ERR");
        Output ("SD:Offline");
        SystemStatusBits |= SSB_SD;  // Turn On Bit     
      } 
    }
    else {
      Output ("SD:Online");
      Output ("SD:OBS DIR Exists");
      SD_exists = true;
    }
  }
}

/* 
 *=======================================================================================================================
 * SD_LogObservation()
 *=======================================================================================================================
 */
void SD_LogObservation(char *observations) {
  char SD_logfile[24];
  File fp;

  if (!SD_exists) {
    return;
  }

  if (!RTC_valid) {
    return;
  }

  // Note: "now" is global and is set when ever timestamp() is called. Value last read from RTC.
  sprintf (SD_logfile, "%s/%4d%02d%02d.log", SD_obsdir, now.year(), now.month(), now.day());

  Output (SD_logfile);
  
  fp = SD.open(SD_logfile, FILE_WRITE); 
  if (fp) {
    fp.println(observations);
    fp.close();
    SystemStatusBits &= ~SSB_SD;  // Turn Off Bit
    Output ("OBS Logged to SD");
  }
  else {
    SystemStatusBits |= SSB_SD;  // Turn On Bit - Note this will be reported on next observation
    Output ("OBS Open Log Err");
    // At thins point we could set SD_exists to false and/or set a status bit to report it
    // SD_initialize();  // Reports SD NOT Found. Library bug with SD
  }
}

/* 
 * =======================================================================================================================
 * Support functions for Config file
 * 
 *  https://arduinogetstarted.com/tutorials/arduino-read-config-from-sd-card
 *  
 *  myInt_1    = SD_findInt(F("myInt_1"));
 *  myFloat_1  = SD_findFloat(F("myFloat_1"));
 *  myString_1 = SD_findString(F("myString_1"));
 *  
 *  CONFIG.TXT content example
 *  myString_1=Hello
 *  myInt_1=2
 *  myFloat_1=0.74
 * =======================================================================================================================
 */

int SD_findKey(const __FlashStringHelper * key, char * value) {
  
  File configFile = SD.open(CF_NAME);

  if (!configFile) {
    Serial.print(F("SD Card: error on opening file "));
    Serial.println(CF_NAME);
    return(0);
  }

  char key_string[KEY_MAX_LENGTH];
  char SD_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1]; // 1 is = character
  int key_length = 0;
  int value_length = 0;

  // Flash string to string
  PGM_P keyPoiter;
  keyPoiter = reinterpret_cast<PGM_P>(key);
  byte ch;
  do {
    ch = pgm_read_byte(keyPoiter++);
    if (ch != 0)
      key_string[key_length++] = ch;
  } while (ch != 0);

  // check line by line
  while (configFile.available()) {
    // UNIX uses LF = \n
    // WINDOWS uses CFLF = \r\n
    int buffer_length = configFile.readBytesUntil('\n', SD_buffer, LINE_MAX_LENGTH);
    if (SD_buffer[buffer_length - 1] == '\r')
      buffer_length--; // trim the \r

    if (buffer_length > (key_length + 1)) { // 1 is = character
      if (memcmp(SD_buffer, key_string, key_length) == 0) { // equal
        if (SD_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, SD_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}

int HELPER_ascii2Int(char *ascii, int length) {
  int sign = 1;
  int number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

long HELPER_ascii2Long(char *ascii, int length) {
  int sign = 1;
  long number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

float HELPER_ascii2Float(char *ascii, int length) {
  int sign = 1;
  int decimalPlace = 0;
  float number  = 0;
  float decimal = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c == '.')
        decimalPlace = 1;
      else if (c >= '0' && c <= '9') {
        if (!decimalPlace)
          number = number * 10 + (c - '0');
        else {
          decimal += ((float)(c - '0') / pow(10.0, decimalPlace));
          decimalPlace++;
        }
      }
    }
  }

  return (number + decimal) * sign;
}

String HELPER_ascii2String(char *ascii, int length) {
  String str;
  str.reserve(length);
  str = "";

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str += String(c);
  }
  return str;
}

char* HELPER_ascii2CharStr(char *ascii, int length) {
  char *str;
  int i = 0;
  str = (char *) malloc (length+1);
  str[0] = 0;
  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str[i] = c;
    str[i+1] = 0;
  }
  return str;
}

bool SD_available(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return value_length > 0;
}

int SD_findInt(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float SD_findFloat(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String SD_findString(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}

char* SD_findCharStr(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2CharStr(value_string, value_length);
}

long SD_findLong(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Long(value_string, value_length);
}


/* 
 * =======================================================================================================================
 * SD_ReadConfigFile()
 * =======================================================================================================================
 */
void SD_ReadConfigFile() {
  cf_ds_type   = SD_findInt(F("ds_type"));
  sprintf(msgbuf, "CF:ds_type=[%d]", cf_ds_type); Output (msgbuf);
}
