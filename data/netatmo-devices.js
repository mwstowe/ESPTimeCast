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
    // Trigger change event to load modules
    const event = new Event('change');
    deviceSelect.dispatchEvent(event);
  }
}

// Function to process Netatmo API response
