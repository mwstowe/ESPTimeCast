// Global variable to store Netatmo devices
window.netatmoDevices = [];

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
  moduleSelect.innerHTML = '<option value="none">Do not use</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Do not use</option>';
  
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
      
      // Handle different response formats
      let devices = [];
      
      if (data.body && data.body.devices) {
        // Standard format
        devices = data.body.devices;
      } else if (data.devices) {
        // Alternative format
        devices = data.devices;
      } else {
        // Try to parse the response as a Netatmo API response
        try {
          const parsedData = typeof data === 'string' ? JSON.parse(data) : data;
          if (parsedData.body && parsedData.body.devices) {
            devices = parsedData.body.devices;
          }
        } catch (e) {
          console.error("Error parsing device data:", e);
        }
      }
      
      if (devices.length === 0) {
        console.log("No devices found in response");
        showStatus("No Netatmo devices found", "error");
        return;
      }
      
      // Store devices in global variable
      window.netatmoDevices = devices;
      
      // Populate device dropdown
      devices.forEach(device => {
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
  
  // Clear existing options but keep the "Do not use" option
  moduleSelect.innerHTML = '<option value="none">Do not use</option>';
  indoorModuleSelect.innerHTML = '<option value="none">Do not use</option>';
  
  // Find the selected device
  const device = window.netatmoDevices.find(d => (d._id === deviceId || d.id === deviceId));
  
  if (!device) {
    console.error("Selected device not found in devices list");
    return;
  }
  
  console.log("Found device:", device);
  
  // Add the main device as an option for indoor module
  const mainOption = document.createElement('option');
  mainOption.value = device._id || device.id;
  mainOption.textContent = "Main Station" + (device.module_name ? ` (${device.module_name})` : "");
  indoorModuleSelect.appendChild(mainOption);
  
  // Check if device has modules
  if (!device.modules || device.modules.length === 0) {
    console.log("No modules found for this device");
    showStatus("No modules found for this device", "error");
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
    if (module.type === 'NAModule4' || module.type === 'NAMain') {
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
