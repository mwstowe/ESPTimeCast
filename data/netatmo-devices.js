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
}// Function to debug Netatmo settings
function debugNetatmoSettings() {
  console.log("Debugging Netatmo settings");
  
  // Log the global devices array
  console.log("Global devices array:", window.netatmoDevices);
  
  // Log the current selections
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  console.log("Current device selection:", deviceSelect ? deviceSelect.value : "Element not found");
  console.log("Current module selection:", moduleSelect ? moduleSelect.value : "Element not found");
  console.log("Current indoor module selection:", indoorModuleSelect ? indoorModuleSelect.value : "Element not found");
  
  // Check if we have device data
  fetch('/netatmo_config.json')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.text();
    })
    .then(text => {
      console.log("Raw config data:", text);
      try {
        const data = JSON.parse(text);
        console.log("Parsed config data:", data);
      } catch (e) {
        console.error("Error parsing config JSON:", e);
      }
    })
    .catch(error => {
      console.log("Error loading config:", error.message);
    });
  
  // Check memory stats
  fetch('/api/system/memory')
    .then(response => response.json())
    .then(data => {
      console.log("Memory stats:", data);
    })
    .catch(error => {
      console.log("Error fetching memory stats:", error.message);
    });
  
  // Show an alert with basic info
  alert("Debug information has been logged to the console. Press F12 to view.");
}

// Make the debug function available globally
window.debugNetatmoSettings = debugNetatmoSettings;            id: module.id,
            module_id: module.module_id,
            mac_address: module.mac_address
          });
        });
      }
    });
  }
}

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
      
      // Clean the response by removing non-JSON characters
      let jsonText = text.replace(/^[^{]*/g, ''); // Remove anything before the first {
      jsonText = jsonText.replace(/[^}]*$/g, ''); // Remove anything after the last }
      
      console.log("Cleaned JSON length:", jsonText.length);
      console.log("First 100 chars:", jsonText.substring(0, 100));
      console.log("Last 100 chars:", jsonText.substring(jsonText.length - 100));
      
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
        throw new Error("Invalid JSON format");
      }
    })
    .catch(error => {
      console.log("No device data found or error loading it:", error.message);
      // This is expected on first load, so we don't show an error
    });
});

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

// Function to fetch Netatmo devices
function fetchNetatmoDevices() {
  console.log("Fetching Netatmo devices...");
  
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
  
  // Show loading message
  showStatus("Refreshing Netatmo stations data...", "loading");
  
  // First, trigger a refresh of the stations data
  fetch('/api/netatmo/refresh-stations')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Refresh initiated:", data);
      
      // Wait longer for the refresh to complete (10 seconds)
      showStatus("Waiting for API call to complete (10 seconds)...", "loading");
      setTimeout(() => {
        // Now get the stations data directly from the file
        fetch('/netatmo_stations_data.json')
          .then(response => {
            if (!response.ok) {
              throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.text(); // Get as text first to clean it
          })
          .then(text => {
            // Clean the response by removing non-JSON characters
            let jsonText = text.replace(/^[^{]*/g, ''); // Remove anything before the first {
            jsonText = jsonText.replace(/[^}]*$/g, ''); // Remove anything after the last }
            
            // Parse the cleaned JSON
            const data = JSON.parse(jsonText);
            console.log("Received stations data");
            
            // Process the device data using our helper function
            let devices = processNetatmoData(data);
            
            // Update the global variable
            window.netatmoDevices = devices;
            
            // Update the UI
            populateDeviceDropdowns(devices);
            
            // Restore previous selection if possible
            if (currentDeviceId) {
              deviceSelect.value = currentDeviceId;
              // Trigger change event to load modules
              const event = new Event('change');
              deviceSelect.dispatchEvent(event);
            }
            
            showStatus("Netatmo stations refreshed successfully!", "success");
            setTimeout(hideStatus, 3000);
          })
          .catch(error => {
            console.error("Error fetching stations data:", error);
            showStatus("Error fetching Netatmo stations: " + error.message, "error");
            setTimeout(hideStatus, 5000);
          });
      }, 10000); // Wait 10 seconds before fetching stations data
    })
    .catch(error => {
      console.error("Error refreshing stations:", error);
      showStatus("Error refreshing Netatmo stations: " + error.message, "error");
      setTimeout(hideStatus, 5000);
    });
}

// Function to populate device dropdowns with the provided devices
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

// Function to process Netatmo API response
function processNetatmoData(data) {
  console.log("Processing Netatmo data:", data);
  
  // Log raw device data to help debug ID issues
  logRawDeviceData(data);
  
  const devices = [];
  
  try {
    // Check if we have a valid response with homes (from gethomedata endpoint)
    if (data && data.body && data.body.homes && Array.isArray(data.body.homes)) {
      console.log("Processing homes data");
      
      // Process each home
      data.body.homes.forEach(home => {
        console.log("Processing home:", home.name);
        
        if (home.modules && Array.isArray(home.modules)) {
          // Find the main station (NAMain)
          const mainModule = home.modules.find(module => module.type === "NAMain");
          
          if (mainModule) {
            console.log("Found main station:", mainModule);
            
            // Create a device for the main station
            const deviceInfo = {
              id: mainModule.id,
              name: mainModule.name || "Weather Station",
              type: mainModule.type,
              modules: []
            };
            
            // Add the main station as a module (for indoor measurements)
            deviceInfo.modules.push({
              id: mainModule.id,
              name: mainModule.name || "Main Module",
              type: mainModule.type,
              data_type: ["Temperature", "Humidity", "CO2", "Pressure", "Noise"]
            });
            
            // Add all other modules
            home.modules.forEach(module => {
              if (module.id !== mainModule.id) {
                console.log("Adding module:", module);
                
                deviceInfo.modules.push({
                  id: module.id,
                  name: module.name || "Unknown Module",
                  type: module.type,
                  data_type: getDataTypeForModuleType(module.type)
                });
              }
            });
            
            devices.push(deviceInfo);
          } else {
            console.warn("No main station (NAMain) found in home:", home.name);
            
            // If no main station is found, still try to add modules
            if (home.modules.length > 0) {
              // Use the first module as the "device"
              const firstModule = home.modules[0];
              
              const deviceInfo = {
                id: firstModule.id,
                name: home.name || "Home",
                type: firstModule.type,
                modules: []
              };
              
              // Add all modules
              home.modules.forEach(module => {
                deviceInfo.modules.push({
                  id: module.id,
                  name: module.name || "Unknown Module",
                  type: module.type,
                  data_type: getDataTypeForModuleType(module.type)
                });
              });
              
              devices.push(deviceInfo);
            }
          }
        } else {
          console.warn("No modules found in home:", home.name);
        }
      });
    }
    }
    // Check if we have a valid response with devices (from getstationsdata endpoint)
    else if (data && data.body && data.body.devices && Array.isArray(data.body.devices)) {
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
            
            // Find the MAC address
            const moduleId = findMacAddress(module);
            
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
          // Create a device for each home
          const homeId = home.id || home.home_id || home._id;
          
          console.log("Using home ID:", homeId);
          
          const deviceInfo = {
            id: homeId,
            name: home.name || "Home",
            type: "NAHome",
            modules: []
          };
          
          // Process modules in the home
          home.modules.forEach(module => {
            // Log the module object to see all available fields
            console.log("Home module object:", module);
            
            // Use the MAC address (id) if available, otherwise fall back to _id
            const moduleId = module.id || module.module_id || module._id;
            
            console.log("Using home module ID:", moduleId);
            
            deviceInfo.modules.push({
              id: moduleId,
              name: module.name || "Unknown Module",
              type: module.type,
              data_type: module.data_type || []
            });
          });
          
          devices.push(deviceInfo);
        }
      });
    }
  } catch (error) {
    console.error("Error processing Netatmo data:", error);
  }
  
  console.log("Processed devices:", devices);
  return devices;
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
  // Find the selected device in our processed data
  const device = window.netatmoDevices.find(d => d.id === deviceId);
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
  if (!device) {
    console.error("Selected device not found in data");
    return;
  }
  
  console.log("Found device:", device);
  console.log("Modules:", device.modules);
  
  // Add outdoor modules (with temperature data)
  device.modules.forEach(module => {
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
    }
    
    // For indoor modules (including the main module)
    if (hasTemperature) {
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name;
      indoorModuleSelect.appendChild(option);
    }
// Function to debug Netatmo settings
function debugNetatmoSettings() {
  console.log("Debugging Netatmo settings...");
  
  // Get current values
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
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
  }
  
  // Show a message to check the console
  showStatus("Debug info logged to console (press F12)", "info");
  setTimeout(hideStatus, 5000);
}  });
  
  // Restore previous selections if possible
  if (currentModuleId) {
    // Check if the value exists in the dropdown
    const moduleExists = Array.from(moduleSelect.options).some(option => option.value === currentModuleId);
    if (moduleExists) {
      moduleSelect.value = currentModuleId;
    } else {
      console.warn(`Module ID ${currentModuleId} not found in dropdown options`);
      moduleSelect.value = 'none';
    }
  }
  
  if (currentIndoorModuleId) {
    // Check if the value exists in the dropdown
    const indoorModuleExists = Array.from(indoorModuleSelect.options).some(option => option.value === currentIndoorModuleId);
    if (indoorModuleExists) {
      indoorModuleSelect.value = currentIndoorModuleId;
    } else {
      console.warn(`Indoor Module ID ${currentIndoorModuleId} not found in dropdown options`);
      // Add the missing module as an option
      const option = document.createElement('option');
      option.value = currentIndoorModuleId;
      option.textContent = `Module ${currentIndoorModuleId} (not found)`;
      indoorModuleSelect.appendChild(option);
      indoorModuleSelect.value = currentIndoorModuleId;
    }
  }
}
