// Helper function to format temperature based on units
String formatTemperature(float temperature, bool roundToInteger) {
  char buffer[10]; // Buffer for formatted temperature
  
  // Apply temperature adjustment if needed
  if (strcmp(weatherUnits, "metric") == 0) {
    temperature += tempAdjust;
    if (roundToInteger) {
      // Format as integer with no decimal places
      sprintf(buffer, "%d", (int)round(temperature));
    } else {
      // Format with exactly one decimal place
      sprintf(buffer, "%.1f", temperature);
      
      // Check if the decimal part is .0 and remove it if so
      int len = strlen(buffer);
      if (len > 2 && buffer[len-2] == '.' && buffer[len-1] == '0') {
        buffer[len-2] = '\0'; // Terminate string before the decimal point
      }
    }
  } else if (strcmp(weatherUnits, "imperial") == 0) {
    // Convert from Celsius to Fahrenheit and apply adjustment
    temperature = (temperature * 9.0 / 5.0) + 32.0 + tempAdjust;
    if (roundToInteger) {
      // Format as integer with no decimal places
      sprintf(buffer, "%d", (int)round(temperature));
    } else {
      // Format with exactly one decimal place
      sprintf(buffer, "%.1f", temperature);
      
      // Check if the decimal part is .0 and remove it if so
      int len = strlen(buffer);
      if (len > 2 && buffer[len-2] == '.' && buffer[len-1] == '0') {
        buffer[len-2] = '\0'; // Terminate string before the decimal point
      }
    }
  } else { // standard (Kelvin)
    temperature = temperature + 273.15 + tempAdjust;
    if (roundToInteger) {
      // Format as integer with no decimal places
      sprintf(buffer, "%d", (int)round(temperature));
    } else {
      // Format with exactly one decimal place
      sprintf(buffer, "%.1f", temperature);
      
      // Check if the decimal part is .0 and remove it if so
      int len = strlen(buffer);
      if (len > 2 && buffer[len-2] == '.' && buffer[len-1] == '0') {
        buffer[len-2] = '\0'; // Terminate string before the decimal point
      }
    }
  }
  
  // Debug output to verify formatting
  Serial.print(F("[FORMAT] Formatted temperature: "));
  Serial.println(buffer);
  
  return String(buffer);
}
