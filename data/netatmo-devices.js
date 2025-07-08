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
  moduleSelect.innerHTML = '<option value="">Select Module...</option>';
  indoorModuleSelect.innerHTML = '<option value="">Select Indoor Module...</option>';
  
  // Show loading message
  showSavingModal("Fetching Netatmo devices...");
  
  // Fetch devices from the API
  fetch('/api/netatmo/devices')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      // Hide loading message
      document.getElementById('savingModal').style.display = 'none';
      document.body.classList.remove('modal-open');
      
      console.log("Received device data from API");
      
      if (!data.body || !data.body.devices || data.body.devices.length === 0) {
        console.log("No devices found in response");
        showStatus("No Netatmo devices found", "error");
        return;
      }
      
      // Store devices in global variable
      window.netatmoDevices = data.body.devices;
      
      // Populate device dropdown
      data.body.devices.forEach(device => {
        const option = document.createElement('option');
        option.value = device._id;
        option.textContent = device.station_name || device.name || device._id;
        deviceSelect.appendChild(option);
      });
      
      // Restore previous selections if they exist
      if (currentDeviceId) {
        deviceSelect.value = currentDeviceId;
        loadModules(currentDeviceId, currentModuleId, currentIndoorModuleId);
      }
      
      console.log(`Found ${data.body.devices.length} Netatmo devices`);
    })
    .catch(error => {
      // Hide loading message
      document.getElementById('savingModal').style.display = 'none';
      document.body.classList.remove('modal-open');
      
      console.error(`Error fetching devices: ${error.message}`);
      showStatus(`Failed to fetch Netatmo devices: ${error.message}`, "error");
    });
}

// Function to load modules for a selected device
function loadModules(deviceId, previousModuleId = null, previousIndoorModuleId = null) {
  console.log(`Loading modules for device: ${deviceId}`);
  
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  if (!moduleSelect || !indoorModuleSelect) {
    console.error("Module select elements not found");
    return;
  }
  
  // Clear existing options
  moduleSelect.innerHTML = '<option value="">Select Module...</option>';
  indoorModuleSelect.innerHTML = '<option value="">Select Indoor Module...</option>';
  
  if (!deviceId) {
    console.log("No device selected");
    return;
  }
  
  // Find the selected device
  const device = window.netatmoDevices.find(d => d._id === deviceId);
  
  if (!device) {
    console.log(`Device not found: ${deviceId}`);
    return;
  }
  
  console.log(`Found device: ${device.station_name || device.name}`);
  
  // Add the main module (base station)
  const mainOption = document.createElement('option');
  mainOption.value = device._id;
  mainOption.textContent = `${device.module_name || 'Base Station'} (Main)`;
  moduleSelect.appendChild(mainOption);
  indoorModuleSelect.appendChild(mainOption.cloneNode(true));
  
  // Add additional modules
  if (device.modules && device.modules.length > 0) {
    device.modules.forEach(module => {
      console.log(`Found module: ${module.module_name} (${module.type})`);
      
      const option = document.createElement('option');
      option.value = module._id;
      option.textContent = `${module.module_name} (${getModuleTypeName(module.type)})`;
      
      // Outdoor modules go to the outdoor dropdown
      if (module.type === 'NAModule1') {
        moduleSelect.appendChild(option);
      }
      
      // Indoor modules go to the indoor dropdown
      if (module.type === 'NAModule4') {
        indoorModuleSelect.appendChild(option.cloneNode(true));
      }
    });
  }
  
  // Restore previous selections if they exist
  if (previousModuleId) {
    moduleSelect.value = previousModuleId;
  }
  
  if (previousIndoorModuleId) {
    indoorModuleSelect.value = previousIndoorModuleId;
  }
}

// Helper function to get human-readable module type names
function getModuleTypeName(type) {
  const types = {
    'NAMain': 'Base Station',
    'NAModule1': 'Outdoor',
    'NAModule2': 'Wind',
    'NAModule3': 'Rain',
    'NAModule4': 'Indoor'
  };
  
  return types[type] || type;
}

// Function to connect to Netatmo
function connectToNetatmo() {
  console.log("Connecting to Netatmo...");
  
  const clientId = document.getElementById('netatmoClientId').value;
  const clientSecret = document.getElementById('netatmoClientSecret').value;
  
  if (!clientId || !clientSecret) {
    console.error("Client ID and Secret are required");
    showStatus("Client ID and Secret are required", "error");
    return;
  }
  
  // Show loading message
  showSavingModal("Connecting to Netatmo...");
  
  // Create form data
  const formData = new FormData();
  formData.append('clientId', clientId);
  formData.append('clientSecret', clientSecret);
  
  // Send credentials to server
  fetch('/api/netatmo/save-credentials', {
    method: 'POST',
    body: formData
  })
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    // The server will redirect to Netatmo auth page
    window.location.href = '/api/netatmo/auth';
  })
  .catch(error => {
    // Hide loading message
    document.getElementById('savingModal').style.display = 'none';
    document.body.classList.remove('modal-open');
    
    console.error(`Error connecting to Netatmo: ${error.message}`);
    showStatus(`Failed to connect to Netatmo: ${error.message}`, "error");
  });
}

// Function to update token status
function updateTokenStatus() {
  console.log("Updating token status...");
  
  // Check if we have an access token by looking at the page data
  fetch('/config.json')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      const tokenStatus = document.getElementById('tokenStatus');
      const connectButton = document.getElementById('connectButton');
      
      if (data.netatmoAccessToken) {
        console.log("Netatmo access token found");
        tokenStatus.textContent = 'Connected to Netatmo';
        tokenStatus.className = 'status-connected';
        
        if (connectButton) {
          connectButton.textContent = 'Reconnect to Netatmo';
        }
      } else {
        console.log("No Netatmo access token found");
        tokenStatus.textContent = 'Not connected to Netatmo';
        tokenStatus.className = 'status-disconnected';
      }
    })
    .catch(error => {
      console.error(`Error checking token status: ${error.message}`);
      const tokenStatus = document.getElementById('tokenStatus');
      tokenStatus.textContent = 'Error checking connection status';
      tokenStatus.className = 'status-error';
    });
}
