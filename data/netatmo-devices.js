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
  showStatus("Fetching Netatmo devices...", "loading");
  
  // Fetch devices from the API with refresh=true to force a new fetch
  fetch('/api/netatmo/devices?refresh=true')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Received device data from API:", data);
      
      // Check if we have a valid response
      if (!data) {
        console.error("No data received from API");
        showStatus("No data received from API", "error");
        return;
      }
      
      // Add more detailed logging
      console.log("Response data type:", typeof data);
      if (typeof data === 'object') {
        console.log("Response keys:", Object.keys(data));
        if (data.body) {
          console.log("Body keys:", Object.keys(data.body));
        }
      }
      
      // Handle the homes format from the homesdata endpoint
      let devices = [];
      
      if (data.body && data.body.homes && data.body.homes.length > 0) {
        console.log("Using homes format from homesdata endpoint");
        
        // Extract the home
        const home = data.body.homes[0];
        console.log("Home:", home);
        
        // Find the main station (NAMain type)
        const mainStation = home.modules.find(module => module.type === "NAMain");
        if (mainStation) {
          console.log("Found main station:", mainStation);
          
          // Create a device object for the main station
          const device = {
            _id: mainStation.id,
            station_name: mainStation.name || home.name,
            type: mainStation.type,
            modules: []
          };
          
          // Add all other modules as sub-modules of the main station
          home.modules.forEach(module => {
            if (module.id !== mainStation.id) {
              device.modules.push({
                _id: module.id,
                module_name: module.name,
                type: module.type,
                room_id: module.room_id
              });
            }
          });
          
          devices.push(device);
        } else {
          // If no main station is found, add all modules as separate devices
          home.modules.forEach(module => {
            devices.push({
              _id: module.id,
              station_name: module.name,
              type: module.type,
              modules: []
            });
          });
        }
      } else if (data.body && data.body.devices) {
        // Standard format
        console.log("Using standard format (body.devices)");
        devices = data.body.devices;
      } else if (data.devices) {
        // Alternative format
        console.log("Using alternative format (devices)");
        devices = data.devices;
      } else {
        // Try to parse the response as a Netatmo API response
        console.log("Trying to parse response as Netatmo API response");
        try {
          const parsedData = typeof data === 'string' ? JSON.parse(data) : data;
          console.log("Parsed data:", parsedData);
          
          if (parsedData.body && parsedData.body.homes && parsedData.body.homes.length > 0) {
            // Extract the home
            const home = parsedData.body.homes[0];
            console.log("Home:", home);
            
            // Find the main station (NAMain type)
            const mainStation = home.modules.find(module => module.type === "NAMain");
            if (mainStation) {
              console.log("Found main station:", mainStation);
              
              // Create a device object for the main station
              const device = {
                _id: mainStation.id,
                station_name: mainStation.name || home.name,
                type: mainStation.type,
                modules: []
              };
              
              // Add all other modules as sub-modules of the main station
              home.modules.forEach(module => {
                if (module.id !== mainStation.id) {
                  device.modules.push({
                    _id: module.id,
                    module_name: module.name,
                    type: module.type,
                    room_id: module.room_id
                  });
                }
              });
              
              devices.push(device);
            } else {
              // If no main station is found, add all modules as separate devices
              home.modules.forEach(module => {
                devices.push({
                  _id: module.id,
                  station_name: module.name,
                  type: module.type,
                  modules: []
                });
              });
            }
          }
        } catch (e) {
          console.error("Error parsing device data:", e);
        }
      }
      
      console.log("Extracted devices:", devices);
      
      if (!devices || devices.length === 0) {
        console.log("No devices found in response");
        showStatus("No Netatmo devices found", "error");
        return;
      }
      
      // Store devices in global variable
      window.netatmoDevices = devices;
      
      // Populate device dropdown
      devices.forEach(device => {
        console.log("Adding device to dropdown:", device);
        const option = document.createElement('option');
        option.value = device._id || device.id;
        option.textContent = device.station_name || device.name || device._id || device.id;
        deviceSelect.appendChild(option);
      });
      
      // Restore previous selections if they exist
      if (currentDeviceId) {
        deviceSelect.value = currentDeviceId;
        loadModules(currentDeviceId, currentModuleId, currentIndoorModuleId);
      }
      
      console.log(`Found ${devices.length} Netatmo devices`);
      showStatus(`Found ${devices.length} Netatmo devices`, "success");
      setTimeout(hideStatus, 3000);
    })
    .catch(error => {
      console.error(`Error fetching devices: ${error.message}`);
      showStatus(`Failed to fetch Netatmo devices: ${error.message}`, "error");
    });
}
// Function to load modules for a selected device
function loadModules(deviceId, selectedModuleId, selectedIndoorModuleId) {
  console.log("Loading modules for device:", deviceId);
  
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!deviceSelect || !moduleSelect || !indoorModuleSelect) {
    console.error("Device or module select elements not found");
    return;
  }
  
  // Use provided deviceId or get from select
  deviceId = deviceId || deviceSelect.value;
  
  if (!deviceId) {
    console.log("No device selected");
    return;
  }
  
  // Clear existing options but keep the "Not mapped" option
  moduleSelect.innerHTML = '<option value="none">Not mapped</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Not mapped</option>';
  
  // Find the selected device
  const device = window.netatmoDevices.find(d => (d._id === deviceId || d.id === deviceId));
  
  if (!device) {
    console.error("Selected device not found in devices list");
    return;
  }
  
  console.log("Found device:", device);
  
  // Add the main device as an option for indoor module if it's a NAMain type
  if (device.type === "NAMain") {
    const mainOption = document.createElement('option');
    mainOption.value = device._id || device.id;
    mainOption.textContent = "Main Station" + (device.station_name ? ` (${device.station_name})` : "");
    indoorModuleSelect.appendChild(mainOption);
  }
  
  // Check if device has modules
  if (!device.modules || device.modules.length === 0) {
    console.log("No modules found for this device");
    showStatus("No modules found for this device", "warning");
    return;
  }
  
  // Process modules
  device.modules.forEach(module => {
    console.log("Processing module:", module);
    
    // For outdoor modules (typically type NAModule1)
    if (module.type === 'NAModule1') {
      const option = document.createElement('option');
      option.value = module._id || module.id;
      option.textContent = module.module_name || module.name || module._id || module.id;
      moduleSelect.appendChild(option);
    }
    
    // For indoor modules (typically type NAModule4 or NAMain)
    if (module.type === 'NAModule4') {
      const option = document.createElement('option');
      option.value = module._id || module.id;
      option.textContent = module.module_name || module.name || module._id || module.id;
      indoorModuleSelect.appendChild(option);
    }
  });
  
  // Restore selections if provided
  if (selectedModuleId) {
    moduleSelect.value = selectedModuleId;
  }
  
  if (selectedIndoorModuleId) {
    indoorModuleSelect.value = selectedIndoorModuleId;
  }
  
  console.log("Modules loaded successfully");
}

// Function to show a saving modal
function showSavingModal(message) {
  let modal = document.getElementById('savingModal');
  
  if (!modal) {
    modal = document.createElement('div');
    modal.id = 'savingModal';
    modal.className = 'modal';
    
    const modalContent = document.createElement('div');
    modalContent.className = 'modal-content';
    
    const spinner = document.createElement('div');
    spinner.className = 'spinner';
    
    const messageElement = document.createElement('p');
    messageElement.id = 'savingModalMessage';
    
    modalContent.appendChild(spinner);
    modalContent.appendChild(messageElement);
    modal.appendChild(modalContent);
    
    document.body.appendChild(modal);
  }
  
  document.getElementById('savingModalMessage').textContent = message;
  modal.style.display = 'flex';
  document.body.classList.add('modal-open');
}
