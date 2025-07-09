// Global variable to store Netatmo devices
window.netatmoDevices = [];

// Make loadModules function available globally
window.loadModulesFromJS = loadModules;

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
      
      // Wait longer for the refresh to complete (5 seconds)
      showStatus("Waiting for API call to complete (5 seconds)...", "loading");
      setTimeout(() => {
        // Now get the stations data
        fetch('/api/netatmo/stations')
          .then(response => {
            if (!response.ok) {
              throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
          })
          .then(data => {
            console.log("Received stations data");
            
            // Process the device data using our helper function
            let devices = processNetatmoData(data);
            
            if (!devices || devices.length === 0) {
              showStatus("No Netatmo devices found", "error");
              setTimeout(hideStatus, 3000);
              return;
            }
            
            // Store devices globally
            window.netatmoDevices = devices;
            
            // Populate device dropdown
            devices.forEach(device => {
              const option = document.createElement('option');
              option.value = device.id;
              option.textContent = device.name;
              deviceSelect.appendChild(option);
            });
            
            // Restore previous selection if it exists
            if (currentDeviceId && deviceSelect.querySelector(`option[value="${currentDeviceId}"]`)) {
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
      }, 5000); // Wait 5 seconds before fetching stations data
    })
    .catch(error => {
      console.error("Error refreshing stations:", error);
      showStatus("Error refreshing Netatmo stations: " + error.message, "error");
      setTimeout(hideStatus, 5000);
    });
}

// Function to load modules for a selected device
function loadModules(deviceId) {
  console.log("Loading modules for device:", deviceId);
  
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!moduleSelect || !indoorModuleSelect) {
    console.error("Module select elements not found");
    return;
  }
  
  // Store current selections
  const currentModuleId = moduleSelect.value;
  const currentIndoorModuleId = indoorModuleSelect.value;
  
  // Clear existing options
  moduleSelect.innerHTML = '<option value="none">Not mapped</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Not mapped</option>';
  
  if (!deviceId || deviceId === "") {
    return;
  }
  
  // Find the selected device
  const device = window.netatmoDevices.find(d => d.id === deviceId);
  if (!device) {
    console.error("Device not found:", deviceId);
    return;
  }
  
  // Populate outdoor module dropdown
  if (device.modules) {
    device.modules.forEach(module => {
      if (module.type === "NAModule1") { // Outdoor module
        const option = document.createElement('option');
        option.value = module.id;
        option.textContent = module.name;
        moduleSelect.appendChild(option);
      }
      
      if (module.type === "NAModule4") { // Indoor module
        const option = document.createElement('option');
        option.value = module.id;
        option.textContent = module.name;
        indoorModuleSelect.appendChild(option);
      }
    });
  }
  
  // Add the main station as an option for indoor readings
  const mainOption = document.createElement('option');
  mainOption.value = device.id;
  mainOption.textContent = device.name + " (Main)";
  indoorModuleSelect.appendChild(mainOption);
  
  // Restore previous selections if they exist
  if (currentModuleId && moduleSelect.querySelector(`option[value="${currentModuleId}"]`)) {
    moduleSelect.value = currentModuleId;
  }
  
  if (currentIndoorModuleId && indoorModuleSelect.querySelector(`option[value="${currentIndoorModuleId}"]`)) {
    indoorModuleSelect.value = currentIndoorModuleId;
  }
}

// Helper function to process Netatmo data
function processNetatmoData(data) {
  console.log("Processing Netatmo data:", data);
  
  if (!data || !data.body) {
    console.error("Invalid data format");
    return [];
  }
  
  const devices = [];
  
  // Process homes data
  if (data.body.homes) {
    data.body.homes.forEach(home => {
      if (home.modules) {
        // Find the main station (NAMain)
        const mainStation = home.modules.find(module => module.type === "NAMain");
        
        if (mainStation) {
          const device = {
            id: mainStation.id,
            name: mainStation.name || "Main Station",
            type: mainStation.type,
            modules: []
          };
          
          // Add connected modules
          home.modules.forEach(module => {
            if (module.type !== "NAMain" && module.bridge === mainStation.id) {
              device.modules.push({
                id: module.id,
                name: module.name || `Module ${module.id}`,
                type: module.type
              });
            }
          });
          
          devices.push(device);
        }
      }
    });
  }
  
  // Process stations data (fallback)
  if (data.body.devices) {
    data.body.devices.forEach(device => {
      const existingDevice = devices.find(d => d.id === device._id);
      
      if (!existingDevice) {
        const newDevice = {
          id: device._id,
          name: device.station_name || device.module_name || "Station",
          type: device.type,
          modules: []
        };
        
        // Add modules
        if (device.modules) {
          device.modules.forEach(module => {
            newDevice.modules.push({
              id: module._id,
              name: module.module_name || `Module ${module._id}`,
              type: module.type
            });
          });
        }
        
        devices.push(newDevice);
      }
    });
  }
  
  console.log("Processed devices:", devices);
  return devices;
}
