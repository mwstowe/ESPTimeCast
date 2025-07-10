// Global variable to store Netatmo devices
window.netatmoDevices = [];

// Make loadModules function available globally
window.loadModulesFromJS = loadModules;

// Function to initialize on page load
document.addEventListener('DOMContentLoaded', function() {
  console.log("Netatmo devices script loaded, attempting to load device data");
  
  // Try to load the device data file directly
  fetch('/netatmo_devices.json')
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
        fetch('/netatmo_devices.json')
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
    deviceSelect.value = currentDeviceId;
    // Manually call loadModules with the current device ID
    loadModules(currentDeviceId, currentModuleId, currentIndoorModuleId);
  }
}

// Function to process Netatmo API response
function processNetatmoData(data) {
  console.log("Processing Netatmo data:", data);
  
  const devices = [];
  
  try {
    // Check if we have a valid response with devices
    if (data && data.body && data.body.devices && Array.isArray(data.body.devices)) {
      // Process each device
      data.body.devices.forEach(device => {
        const deviceInfo = {
          id: device._id,
          name: device.station_name || device.module_name || "Unknown Device",
          type: device.type,
          modules: []
        };
        
        // Add the main device as a module (for indoor measurements)
        deviceInfo.modules.push({
          id: device._id,
          name: device.module_name || "Main Module",
          type: device.type,
          data_type: device.data_type || []
        });
        
        // Process modules if available
        if (device.modules && Array.isArray(device.modules)) {
          device.modules.forEach(module => {
            deviceInfo.modules.push({
              id: module._id,
              name: module.module_name || "Unknown Module",
              type: module.type,
              data_type: module.data_type || []
            });
          });
        }
        
        devices.push(deviceInfo);
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
  });
  
  // Restore previous selections if possible
  if (currentModuleId) {
    moduleSelect.value = currentModuleId || 'none';
  }
  
  if (currentIndoorModuleId) {
    indoorModuleSelect.value = currentIndoorModuleId || 'none';
  }
}

// Helper function to show status messages
function showStatus(message, type) {
  const statusDiv = document.getElementById('statusMessage');
  if (!statusDiv) return;
  
  statusDiv.textContent = message;
  statusDiv.className = 'status-message ' + (type || 'info');
  statusDiv.style.display = 'block';
}

// Helper function to hide status messages
function hideStatus() {
  const statusDiv = document.getElementById('statusMessage');
  if (statusDiv) {
    statusDiv.style.display = 'none';
  }
}
