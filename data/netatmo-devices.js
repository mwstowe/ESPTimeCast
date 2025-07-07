// Function to handle Netatmo device selection
document.addEventListener('DOMContentLoaded', function() {
  // Get form elements
  const configForm = document.getElementById('configForm');
  
  // Add event listener to form submission
  if (configForm) {
    const originalSubmit = configForm.onsubmit;
    configForm.onsubmit = function(e) {
      // Get values from manual input fields
      const deviceIdManual = document.getElementById('netatmoDeviceIdManual');
      const moduleIdManual = document.getElementById('netatmoModuleIdManual');
      const indoorModuleIdManual = document.getElementById('netatmoIndoorModuleIdManual');
      
      // Get the select elements
      const deviceIdSelect = document.getElementById('netatmoDeviceId');
      const moduleIdSelect = document.getElementById('netatmoModuleId');
      const indoorModuleIdSelect = document.getElementById('netatmoIndoorModuleId');
      
      // Create hidden fields for the actual values
      const deviceIdHidden = document.createElement('input');
      deviceIdHidden.type = 'hidden';
      deviceIdHidden.name = 'netatmoDeviceId';
      deviceIdHidden.value = deviceIdManual.value;
      configForm.appendChild(deviceIdHidden);
      
      const moduleIdHidden = document.createElement('input');
      moduleIdHidden.type = 'hidden';
      moduleIdHidden.name = 'netatmoModuleId';
      moduleIdHidden.value = moduleIdManual.value;
      configForm.appendChild(moduleIdHidden);
      
      const indoorModuleIdHidden = document.createElement('input');
      indoorModuleIdHidden.type = 'hidden';
      indoorModuleIdHidden.name = 'netatmoIndoorModuleId';
      indoorModuleIdHidden.value = indoorModuleIdManual.value;
      configForm.appendChild(indoorModuleIdHidden);
      
      // Call the original submit handler
      if (originalSubmit) {
        return originalSubmit.call(this, e);
      }
    };
  }
  
  // Load config and populate fields
  fetch('/config.json')
    .then(response => response.json())
    .then(data => {
      // Populate manual fields with saved values
      const deviceIdManual = document.getElementById('netatmoDeviceIdManual');
      const moduleIdManual = document.getElementById('netatmoModuleIdManual');
      const indoorModuleIdManual = document.getElementById('netatmoIndoorModuleIdManual');
      
      if (deviceIdManual) deviceIdManual.value = data.netatmoDeviceId || '';
      if (moduleIdManual) moduleIdManual.value = data.netatmoModuleId || '';
      if (indoorModuleIdManual) indoorModuleIdManual.value = data.netatmoIndoorModuleId || '';
      
      // Try to fetch devices if we have credentials
      if (data.netatmoClientId && data.netatmoClientSecret && 
          data.netatmoUsername && data.netatmoPassword) {
        fetchNetatmoDevices();
      }
    })
    .catch(err => {
      console.error('Failed to load config:', err);
    });
});

// Function to fetch Netatmo devices
function fetchNetatmoDevices() {
  console.log("Fetching Netatmo devices...");
  fetch('/api/netatmo/devices')
    .then(response => response.json())
    .then(data => {
      if (data.error) {
        console.error("Error fetching Netatmo devices:", data.error);
        return;
      }
      
      const deviceSelect = document.getElementById('netatmoDeviceId');
      const moduleSelect = document.getElementById('netatmoModuleId');
      const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
      
      if (!deviceSelect || !moduleSelect || !indoorModuleSelect) {
        console.error("Could not find select elements");
        return;
      }
      
      // Clear existing options
      deviceSelect.innerHTML = '<option value="">Select a device...</option>';
      moduleSelect.innerHTML = '<option value="">Select a module...</option>';
      indoorModuleSelect.innerHTML = '<option value="">Select a module...</option>';
      
      // Add devices to the dropdown
      if (data.devices && data.devices.length > 0) {
        data.devices.forEach(device => {
          const option = document.createElement('option');
          option.value = device.id;
          option.textContent = device.name;
          option.dataset.modules = JSON.stringify(device.modules);
          deviceSelect.appendChild(option);
        });
        
        // If we have a saved device ID, select it
        const savedDeviceId = document.getElementById('netatmoDeviceIdManual').value;
        if (savedDeviceId) {
          deviceSelect.value = savedDeviceId;
          // Trigger change event to populate modules
          deviceSelect.dispatchEvent(new Event('change'));
        }
      }
    })
    .catch(error => {
      console.error('Error fetching Netatmo devices:', error);
    });
}
