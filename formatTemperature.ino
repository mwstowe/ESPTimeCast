// Helper function to format temperature based on units
String formatTemperature(float temperature, bool roundToInteger) {
  String result;
  
  // Apply temperature adjustment if needed
  if (strcmp(weatherUnits, "metric") == 0) {
    temperature += tempAdjust;
    if (roundToInteger) {
      result = String(round(temperature));
    } else {
      result = String(temperature, 1);
      result.replace(".0", "");
    }
  } else if (strcmp(weatherUnits, "imperial") == 0) {
    // Convert from Celsius to Fahrenheit and apply adjustment
    temperature = (temperature * 9.0 / 5.0) + 32.0 + tempAdjust;
    if (roundToInteger) {
      result = String(round(temperature));
    } else {
      result = String(temperature, 1);
      result.replace(".0", "");
    }
  } else { // standard (Kelvin)
    temperature = temperature + 273.15 + tempAdjust;
    if (roundToInteger) {
      result = String(round(temperature));
    } else {
      result = String(temperature, 1);
      result.replace(".0", "");
    }
  }
  
  return result;
}
