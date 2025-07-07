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
  
  fetch('/api/netatmo/devices')
    .then(response => {
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      return response.json();
    })
    .then(data => {
      if (data.error) {
        throw new Error(data.error);
      }
      
      // Hide loading message
      document.getElementById('savingModal').style.display = 'none';
      document.body.classList.remove('modal-open');
      
      // Store devices globally
      window.netatmoDevices = data.devices || [];
      
      // Populate device dropdown
      if (data.devices && data.devices.length > 0) {
        data.devices.forEach(device => {
          const option = document.createElement('option');
          option.value = device.id;
          option.textContent = device.name || device.id;
          deviceSelect.appendChild(option);
        });
        
        // Restore previous selection if possible
        if (currentDeviceId) {
          deviceSelect.value = currentDeviceId;
          // Trigger change to load modules
          loadModules();
          
          // Restore module selections
          if (currentModuleId) {
            moduleSelect.value = currentModuleId;
          }
          if (currentIndoorModuleId) {
            indoorModuleSelect.value = currentIndoorModuleId;
          }
        }
      } else {
        alert('No Netatmo devices found');
      }
    })
    .catch(error => {
      // Hide loading message
      document.getElementById('savingModal').style.display = 'none';
      document.body.classList.remove('modal-open');
      
      // Show error
      alert('Error fetching Netatmo devices: ' + error.message);
    });
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
  
  // Get the device data from the previously loaded devices
  const deviceData = window.netatmoDevices.find(d => d.id === deviceId);
  if (!deviceData || !deviceData.modules) return;
  
  // Populate outdoor modules
  deviceData.modules.forEach(module => {
    if (module.type === 'NAModule1') { // Outdoor module
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name || module.id;
      moduleSelect.appendChild(option);
    } else if (module.type === 'NAModule4') { // Indoor module
      const option = document.createElement('option');
      option.value = module.id;
      option.textContent = module.name || module.id;
      indoorModuleSelect.appendChild(option);
    }
  });
}
