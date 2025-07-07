// Global variable to store Netatmo devices
window.netatmoDevices = [];

// Function to fetch Netatmo devices
function fetchNetatmoDevices() {
  const deviceSelect = document.getElementById('netatmoDeviceId');
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
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
  
  // In this simplified version, we'll just use mock data
  setTimeout(() => {
    // Hide loading message
    document.getElementById('savingModal').style.display = 'none';
    document.body.classList.remove('modal-open');
    
    // Mock data
    const mockData = {
      devices: [
        {
          id: "manual",
          name: "Manual Configuration",
          type: "station",
          modules: [
            {
              id: "manual_main",
              name: "Main Station",
              type: "NAMain"
            },
            {
              id: "manual_outdoor",
              name: "Outdoor Module",
              type: "NAModule1"
            },
            {
              id: "manual_indoor",
              name: "Indoor Module",
              type: "NAModule4"
            }
          ]
        }
      ]
    };
    
    // Store devices globally
    window.netatmoDevices = mockData.devices;
    
    // Populate device dropdown
    mockData.devices.forEach(device => {
      const option = document.createElement('option');
      option.value = device.id;
      option.textContent = device.name;
      deviceSelect.appendChild(option);
    });
    
    // Select the first device
    deviceSelect.value = "manual";
    
    // Load modules
    loadModules();
  }, 500);
}

// Function to load modules for selected device
function loadModules() {
  const deviceId = document.getElementById('netatmoDeviceId').value;
  const moduleSelect = document.getElementById('netatmoModuleId');
  const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
  
  // Clear existing options
  moduleSelect.innerHTML = '<option value="">Select Module...</option>';
  indoorModuleSelect.innerHTML = '<option value="">Select Indoor Module...</option>';
  
  if (!deviceId) return;
  
  // Get the device data
  const deviceData = window.netatmoDevices.find(d => d.id === deviceId);
  if (!deviceData || !deviceData.modules) return;
  
  // Populate modules
  deviceData.modules.forEach(module => {
    if (module.type === 'NAModule1') { // Outdoor module
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name;
      moduleSelect.appendChild(option);
    } else if (module.type === 'NAModule4' || module.type === 'NAMain') { // Indoor module or main station
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name;
      indoorModuleSelect.appendChild(option);
    }
  });
}
