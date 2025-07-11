// Function to check if a string is in MAC address format (xx:xx:xx:xx:xx:xx)
function isMacAddressFormat(str) {
  return /^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/.test(str);
}

// Function to find the MAC address field in a device object
function findMacAddress(obj) {
  // Check common field names for MAC addresses
  const possibleFields = ['mac_address', 'macAddress', 'MAC', 'mac', 'id', 'device_id', '_id'];
  
  for (const field of possibleFields) {
    if (obj[field] && typeof obj[field] === 'string') {
      if (isMacAddressFormat(obj[field])) {
        console.log(`Found MAC address in field '${field}':`, obj[field]);
        return obj[field];
      }
    }
  }
  
  // If no MAC address format found, look for any field that might contain a MAC
  for (const key in obj) {
    if (typeof obj[key] === 'string' && isMacAddressFormat(obj[key])) {
      console.log(`Found MAC address in field '${key}':`, obj[key]);
      return obj[key];
    }
  }
  
  // If still not found, return the _id as fallback
  return obj._id || '';
}

// Function to log raw device data
function logRawDeviceData(data) {
  console.log("Raw device data:", data);
  
  if (data && data.body && data.body.devices && Array.isArray(data.body.devices)) {
    data.body.devices.forEach((device, index) => {
      console.log(`Device ${index}:`, device);
      console.log(`Device ${index} ID fields:`, {
        _id: device._id,
        id: device.id,
        device_id: device.device_id,
        mac_address: device.mac_address
      });
      
      if (device.modules && Array.isArray(device.modules)) {
        device.modules.forEach((module, moduleIndex) => {
          console.log(`Device ${index} Module ${moduleIndex}:`, module);
          console.log(`Device ${index} Module ${moduleIndex} ID fields:`, {
            _id: module._id,
            id: module.id,
            module_id: module.module_id,
            mac_address: module.mac_address
          });
        });
      }
    });
  }
}

// Helper function to get data types for module types
function getDataTypeForModuleType(type) {
  switch (type) {
    case "NAMain":
      return ["Temperature", "Humidity", "CO2", "Pressure", "Noise"];
    case "NAModule1":
      return ["Temperature", "Humidity"];
    case "NAModule2":
      return ["Wind"];
    case "NAModule3":
      return ["Rain"];
    case "NAModule4":
      return ["Temperature", "Humidity", "CO2"];
    default:
      return ["Temperature"]; // Default to temperature for unknown modules
  }
}

// Function to process Netatmo API response
function processNetatmoData(data) {
  console.log("Processing Netatmo data:", data);
  
  // Log raw device data to help debug ID issues
  logRawDeviceData(data);
  
  const devices = [];
  
  try {
    // Check if we have a valid response with devices (from getstationsdata endpoint)
    if (data && data.body && data.body.devices && Array.isArray(data.body.devices)) {
      // Process each device from getstationsdata endpoint
      data.body.devices.forEach(device => {
        // Log the device object to see all available fields
        console.log("Device object:", device);
        
        // Find the MAC address
        const deviceId = findMacAddress(device);
        
        const deviceInfo = {
          id: deviceId,
          name: device.station_name || device.module_name || "Unknown Device",
          type: device.type,
          modules: []
        };
        
        console.log("Using device ID:", deviceId);
        
        // Add the main device as a module (for indoor measurements)
        deviceInfo.modules.push({
          id: deviceId,
          name: device.module_name || "Main Module",
          type: device.type,
          data_type: device.data_type || []
        });
        
        // Process modules if available
        if (device.modules && Array.isArray(device.modules)) {
          device.modules.forEach(module => {
            // Log the module object to see all available fields
            console.log("Module object:", module);
            
            // Use the MAC address (id) if available, otherwise fall back to _id
            const moduleId = module.id || module.module_id || module._id;
            
            console.log("Using module ID:", moduleId);
            
            deviceInfo.modules.push({
              id: moduleId,
              name: module.module_name || "Unknown Module",
              type: module.type,
              data_type: module.data_type || []
            });
          });
        }
        
        devices.push(deviceInfo);
      });
    }
    // Check if we have a valid response with homes (from gethomedata endpoint)
    else if (data && data.body && data.body.homes && Array.isArray(data.body.homes)) {
      console.log("Processing homes data");
      
      // Process each home
      data.body.homes.forEach(home => {
        // Log the home object to see all available fields
        console.log("Home object:", home);
        
        if (home.modules && Array.isArray(home.modules)) {
          // Find the main station (NAMain) to use as the device
          const mainModule = home.modules.find(module => module.type === "NAMain");
          
          if (mainModule) {
            // Use the MAC address of the main module as the device ID
            const deviceId = mainModule.id || '';
            
            console.log("Found main module with ID:", deviceId);
            
            // Create a device using the main module's ID
            const deviceInfo = {
              id: deviceId, // Use the main module's ID instead of the home ID
              name: home.name || "Home",
              type: "NAMain",
              homeId: home.id, // Store the home ID for reference
              modules: []
            };
            
            // Add all modules to the device
            home.modules.forEach(module => {
              console.log("Processing module:", module);
              
              const moduleId = module.id || '';
              
              deviceInfo.modules.push({
                id: moduleId,
                name: module.name || "Unknown Module",
                type: module.type,
                data_type: getDataTypeForModuleType(module.type)
              });
            });
            
            devices.push(deviceInfo);
          } else {
            console.warn("No main module (NAMain) found in home:", home.name);
            
            // If no main module is found, use the first module with a MAC address
            const moduleWithMac = home.modules.find(module => isMacAddressFormat(module.id));
            
            if (moduleWithMac) {
              const deviceId = moduleWithMac.id;
              console.log("Using module with MAC address as device:", deviceId);
              
              const deviceInfo = {
                id: deviceId,
                name: home.name || "Home",
                type: moduleWithMac.type,
                homeId: home.id,
                modules: []
              };
              
              // Add all modules
              home.modules.forEach(module => {
                deviceInfo.modules.push({
                  id: module.id || '',
                  name: module.name || "Unknown Module",
                  type: module.type,
                  data_type: module.data_type || getDataTypeForModuleType(module.type)
                });
              });
              
              devices.push(deviceInfo);
            } else {
              console.error("No module with MAC address found in home:", home.name);
              
              // As a last resort, create a device with the home ID but warn about it
              console.warn("Using home ID as device ID (not recommended):", home.id);
              
              const deviceInfo = {
                id: home.id,
                name: home.name || "Home",
                type: "NAHome",
                isHomeId: true, // Flag to indicate this is a home ID, not a device ID
                modules: []
              };
              
              // Add all modules
              home.modules.forEach(module => {
                deviceInfo.modules.push({
                  id: module.id || '',
                  name: module.name || "Unknown Module",
                  type: module.type,
                  data_type: module.data_type || getDataTypeForModuleType(module.type)
                });
              });
              
              devices.push(deviceInfo);
            }
          }
        }
      });
    }
  } catch (error) {
    console.error("Error processing Netatmo data:", error);
  }
  
  console.log("Processed devices:", devices);
  
  // Check if any devices have MAC address IDs
  const macDevices = devices.filter(d => isMacAddressFormat(d.id));
  if (macDevices.length > 0) {
    console.log("Found devices with MAC address IDs:", macDevices);
    return macDevices; // Only return devices with MAC address IDs
  }
  
  return devices;
}

// Function to populate device dropdowns
function populateDeviceDropdowns(devices) {
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!deviceSelect || !moduleSelect || !indoorModuleSelect) {
    console.error("Device or module select elements not found");
    return;
  }
  
  // Store current selections
  const currentDeviceId = deviceSelect.value;
  const currentModuleId = moduleSelect.value;
  const currentIndoorModuleId = indoorModuleSelect.value;
  
  // Clear existing options
  deviceSelect.innerHTML = '<option value="">Select Device...</option>';
  moduleSelect.innerHTML = '<option value="none">Not mapped</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Not mapped</option>';
  
  // Add device options
  devices.forEach(device => {
    const option = document.createElement('option');
    option.value = device.id;
    option.textContent = device.name;
    deviceSelect.appendChild(option);
  });
  
  // Restore previous selections if possible
  if (currentDeviceId) {
    // Check if the value exists in the dropdown
    const deviceExists = Array.from(deviceSelect.options).some(option => option.value === currentDeviceId);
    if (deviceExists) {
      deviceSelect.value = currentDeviceId;
      // Manually call loadModules with the current device ID
      loadModules(currentDeviceId, currentModuleId, currentIndoorModuleId);
    } else {
      console.warn(`Device ID ${currentDeviceId} not found in dropdown options`);
      // Add the missing device as an option
      const option = document.createElement('option');
      option.value = currentDeviceId;
      option.textContent = `Device ${currentDeviceId} (not found)`;
      deviceSelect.appendChild(option);
      deviceSelect.value = currentDeviceId;
      
      // Create empty modules for this device
      const dummyDevice = {
        id: currentDeviceId,
        name: `Device ${currentDeviceId} (not found)`,
        modules: []
      };
      
      // If we have module IDs, add them as dummy modules
      if (currentModuleId && currentModuleId !== 'none') {
        dummyDevice.modules.push({
          id: currentModuleId,
          name: `Module ${currentModuleId} (not found)`,
          type: 'NAModule1'
        });
      }
      
      if (currentIndoorModuleId && currentIndoorModuleId !== 'none') {
        dummyDevice.modules.push({
          id: currentIndoorModuleId,
          name: `Module ${currentIndoorModuleId} (not found)`,
          type: 'NAModule4'
        });
      }
      
      // Add the dummy device to the global devices array
      window.netatmoDevices.push(dummyDevice);
      
      // Manually call loadModules with the current device ID
      loadModules(currentDeviceId, currentModuleId, currentIndoorModuleId);
    }
  }
}

// Function to load modules for a selected device
function loadModules(deviceId, currentModuleId, currentIndoorModuleId) {
  console.log("Loading modules for device:", deviceId);
  console.log("Current module ID:", currentModuleId);
  console.log("Current indoor module ID:", currentIndoorModuleId);
  console.log("Current global devices:", window.netatmoDevices);
  
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!moduleSelect || !indoorModuleSelect) {
    console.error("Module select elements not found");
    return;
  }
  
  // Clear existing options
  moduleSelect.innerHTML = '<option value="none">Not mapped</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Not mapped</option>';
  
  if (!deviceId) return;
  
  // Find the selected device in our processed data
  const device = window.netatmoDevices.find(d => d.id === deviceId);
  
  if (!device) {
    console.error("Selected device not found in data");
    return;
  }
  
  console.log("Found device:", device);
  console.log("Modules:", device.modules);
  
  // Add outdoor modules (with temperature data)
  device.modules.forEach(module => {
    console.log("Processing module for dropdowns:", module);
    
    // Check if this module has temperature data
    const hasTemperature = module.data_type && 
                          (module.data_type.includes('Temperature') || 
                           module.data_type.includes('temperature'));
    
    // For outdoor modules (typically NAModule1)
    if (module.type === 'NAModule1' || (module.type !== 'NAMain' && hasTemperature)) {
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name;
      moduleSelect.appendChild(option);
      console.log("Added to outdoor dropdown:", module.name, module.id);
    }
    
    // For indoor modules (including the main module)
    if (hasTemperature || module.type === 'NAMain' || module.type === 'NAModule4') {
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name;
      indoorModuleSelect.appendChild(option);
      console.log("Added to indoor dropdown:", module.name, module.id);
    }
  });
  
  // Set the selected values if provided
  if (currentModuleId && currentModuleId !== 'none') {
    // Check if the value exists in the dropdown
    const moduleExists = Array.from(moduleSelect.options).some(option => option.value === currentModuleId);
    if (moduleExists) {
      moduleSelect.value = currentModuleId;
      console.log("Set outdoor module to:", currentModuleId);
    } else {
      console.warn(`Module ID ${currentModuleId} not found in outdoor dropdown options`);
    }
  }
  
  if (currentIndoorModuleId && currentIndoorModuleId !== 'none') {
    // Check if the value exists in the dropdown
    const indoorModuleExists = Array.from(indoorModuleSelect.options).some(option => option.value === currentIndoorModuleId);
    if (indoorModuleExists) {
      indoorModuleSelect.value = currentIndoorModuleId;
      console.log("Set indoor module to:", currentIndoorModuleId);
    } else {
      console.warn(`Indoor module ID ${currentIndoorModuleId} not found in indoor dropdown options`);
    }
  }
}

// Function to log dropdown contents for debugging
function logDropdownContents() {
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  console.log("Device dropdown options:");
  if (deviceSelect) {
    Array.from(deviceSelect.options).forEach(option => {
      console.log(`  ${option.value}: ${option.textContent}`);
    });
  }
  
  console.log("Outdoor module dropdown options:");
  if (moduleSelect) {
    Array.from(moduleSelect.options).forEach(option => {
      console.log(`  ${option.value}: ${option.textContent}`);
    });
  }
  
  console.log("Indoor module dropdown options:");
  if (indoorModuleSelect) {
    Array.from(indoorModuleSelect.options).forEach(option => {
      console.log(`  ${option.value}: ${option.textContent}`);
    });
  }
}

// Function to debug Netatmo settings
function debugNetatmoSettings() {
  console.log("Debugging Netatmo settings...");
  
  // Get current values
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!deviceSelect || !moduleSelect || !indoorModuleSelect) {
    console.error("One or more dropdown elements not found");
    alert("Error: Could not find dropdown elements. Check the console for details.");
    return;
  }
  
  console.log("Current device ID:", deviceSelect.value);
  console.log("Current module ID:", moduleSelect.value);
  console.log("Current indoor module ID:", indoorModuleSelect.value);
  
  console.log("Device options:");
  Array.from(deviceSelect.options).forEach(option => {
    console.log(`  ${option.value}: ${option.textContent}`);
  });
  
  console.log("Module options:");
  Array.from(moduleSelect.options).forEach(option => {
    console.log(`  ${option.value}: ${option.textContent}`);
  });
  
  console.log("Indoor module options:");
  Array.from(indoorModuleSelect.options).forEach(option => {
    console.log(`  ${option.value}: ${option.textContent}`);
  });
  
  console.log("Global Netatmo devices:", window.netatmoDevices);
  
  // Check if the current device exists in the global devices array
  const currentDeviceId = deviceSelect.value;
  const device = window.netatmoDevices.find(d => d.id === currentDeviceId);
  console.log("Current device in global array:", device);
  
  // Check if the current module exists in the current device's modules
  if (device) {
    const currentModuleId = moduleSelect.value;
    const module = device.modules.find(m => m.id === currentModuleId);
    console.log("Current module in device's modules:", module);
    
    const currentIndoorModuleId = indoorModuleSelect.value;
    const indoorModule = device.modules.find(m => m.id === currentIndoorModuleId);
    console.log("Current indoor module in device's modules:", indoorModule);
    
    // Check if the device ID is a MAC address
    if (isMacAddressFormat(currentDeviceId)) {
      console.log("Device ID is a valid MAC address format");
    } else {
      console.warn("Device ID is NOT in MAC address format");
    }
    
    // Check if the module IDs are MAC addresses
    if (currentModuleId !== 'none') {
      if (isMacAddressFormat(currentModuleId)) {
        console.log("Module ID is a valid MAC address format");
      } else {
        console.warn("Module ID is NOT in MAC address format");
      }
    }
    
    if (currentIndoorModuleId !== 'none') {
      if (isMacAddressFormat(currentIndoorModuleId)) {
        console.log("Indoor module ID is a valid MAC address format");
      } else {
        console.warn("Indoor module ID is NOT in MAC address format");
      }
    }
  }
  
  // Check if we have device data
  fetch('/netatmo_config.json')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.text();
    })
    .then(text => {
      console.log("Raw config data length:", text.length);
      console.log("First 100 characters:", text.substring(0, 100));
      
      // Clean the response by removing chunked transfer encoding artifacts
      let cleanedText = text;
      
      // Remove hex chunk size markers at the beginning (like "55a")
      if (/^[0-9a-fA-F]+\r?\n/.test(cleanedText)) {
        console.log("Found hex chunk marker at the beginning");
        cleanedText = cleanedText.replace(/^[0-9a-fA-F]+\r?\n/, '');
      }
      
      // Remove the trailing "0" chunk marker if present
      if (/\r?\n0\r?\n\r?\n$/.test(cleanedText)) {
        console.log("Found trailing chunk marker");
        cleanedText = cleanedText.replace(/\r?\n0\r?\n\r?\n$/, '');
      }
      
      try {
        const data = JSON.parse(cleanedText);
        console.log("Parsed config data:", data);
        
        // Process the data and update the UI
        const devices = processNetatmoData(data);
        console.log("Processed devices:", devices);
        
        // Check if any of the devices have a MAC address format ID
        const macDevices = devices.filter(d => isMacAddressFormat(d.id));
        console.log("Devices with MAC address format IDs:", macDevices);
        
        // Update the UI with the processed data
        if (macDevices.length > 0) {
          window.netatmoDevices = devices;
          populateDeviceDropdowns(devices);
          alert("Debug information has been logged to the console. Found " + macDevices.length + " devices with MAC addresses. Press F12 to view.");
        } else {
          alert("Debug information has been logged to the console. WARNING: No devices with MAC address format IDs were found. Press F12 to view.");
        }
      } catch (e) {
        console.error("Error parsing config JSON:", e);
        alert("Error parsing JSON. Check the console for details.");
      }
    })
    .catch(error => {
      console.error("Error fetching config:", error);
      alert("Error fetching config: " + error.message);
    });
}

// Make the debug function available globally
window.debugNetatmoSettings = debugNetatmoSettings;

// Global variable to store Netatmo devices
window.netatmoDevices = [];

// Make loadModules function available globally
window.loadModulesFromJS = loadModules;

// Function to initialize on page load
document.addEventListener('DOMContentLoaded', function() {
  console.log("Netatmo devices script loaded, attempting to load device data");
  
  // Try to load the device data file directly
  fetch('/netatmo_config.json')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.text(); // Get as text first to clean it
    })
    .then(text => {
      console.log("Raw response length:", text.length);
      console.log("First 100 chars:", text.substring(0, 100));
      
      // Clean the response by removing chunked transfer encoding artifacts
      let cleanedText = text;
      
      // Remove hex chunk size markers at the beginning (like "55a")
      if (/^[0-9a-fA-F]+\r?\n/.test(cleanedText)) {
        console.log("Found hex chunk marker at the beginning");
        cleanedText = cleanedText.replace(/^[0-9a-fA-F]+\r?\n/, '');
      }
      
      // Remove the trailing "0" chunk marker if present
      if (/\r?\n0\r?\n\r?\n$/.test(cleanedText)) {
        console.log("Found trailing chunk marker");
        cleanedText = cleanedText.replace(/\r?\n0\r?\n\r?\n$/, '');
      }
      
      // Now remove anything before the first { and after the last }
      let jsonStartIndex = cleanedText.indexOf('{');
      let jsonEndIndex = cleanedText.lastIndexOf('}');
      
      if (jsonStartIndex >= 0 && jsonEndIndex >= 0 && jsonEndIndex > jsonStartIndex) {
        let jsonText = cleanedText.substring(jsonStartIndex, jsonEndIndex + 1);
        
        console.log("Cleaned JSON length:", jsonText.length);
        console.log("First 100 chars of cleaned JSON:", jsonText.substring(0, 100));
        console.log("Last 100 chars of cleaned JSON:", jsonText.substring(jsonText.length - 100));
        
        try {
          // Parse the cleaned JSON
          const data = JSON.parse(jsonText);
          console.log("Found existing device data");
          
          // Process the device data using our helper function
          let devices = processNetatmoData(data);
          
          // Update the global variable
          window.netatmoDevices = devices;
          
          // Update the UI with the device data
          populateDeviceDropdowns(devices);
          
          // Log the populated dropdowns
          logDropdownContents();
        } catch (e) {
          console.error("Error parsing JSON:", e);
          console.error("JSON text that failed to parse:", jsonText);
          throw new Error("Invalid JSON format");
        }
      } else {
        console.error("Could not find valid JSON markers in the response");
        throw new Error("Invalid response format");
      }
    })
    .catch(error => {
      console.log("No device data found or error loading it:", error.message);
      // This is expected on first load, so we don't show an error
    });
});
