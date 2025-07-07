// Function to handle Netatmo device selection
document.addEventListener('DOMContentLoaded', function() {
  // Get form elements
  const configForm = document.getElementById('configForm');
  
  // Get the input fields
  const deviceIdManual = document.getElementById('netatmoDeviceIdManual');
  const moduleIdManual = document.getElementById('netatmoModuleIdManual');
  const indoorModuleIdManual = document.getElementById('netatmoIndoorModuleIdManual');
  
  // Get the select elements
  const deviceIdSelect = document.getElementById('netatmoDeviceId');
  const moduleIdSelect = document.getElementById('netatmoModuleId');
  const indoorModuleIdSelect = document.getElementById('netatmoIndoorModuleId');
  
  // Get the hidden fields
  const deviceIdActual = document.getElementById('netatmoDeviceIdActual');
  const moduleIdActual = document.getElementById('netatmoModuleIdActual');
  const indoorModuleIdActual = document.getElementById('netatmoIndoorModuleIdActual');
  
  // Get the authorize button
  const authorizeBtn = document.getElementById('authorizeNetatmo');
  
  // Function to update hidden fields
  function updateHiddenFields() {
    if (deviceIdActual) {
      deviceIdActual.value = deviceIdManual.value || deviceIdSelect.value;
    }
    if (moduleIdActual) {
      moduleIdActual.value = moduleIdManual.value || moduleIdSelect.value;
    }
    if (indoorModuleIdActual) {
      indoorModuleIdActual.value = indoorModuleIdManual.value || indoorModuleIdSelect.value;
    }
    
    console.log("Updated hidden fields:", 
                deviceIdActual ? deviceIdActual.value : 'N/A',
                moduleIdActual ? moduleIdActual.value : 'N/A',
                indoorModuleIdActual ? indoorModuleIdActual.value : 'N/A');
  }
  
  // Add event listeners to update hidden fields
  if (deviceIdManual) {
    deviceIdManual.addEventListener('input', updateHiddenFields);
  }
  if (moduleIdManual) {
    moduleIdManual.addEventListener('input', updateHiddenFields);
  }
  if (indoorModuleIdManual) {
    indoorModuleIdManual.addEventListener('input', updateHiddenFields);
  }
  if (deviceIdSelect) {
    deviceIdSelect.addEventListener('change', updateHiddenFields);
  }
  if (moduleIdSelect) {
    moduleIdSelect.addEventListener('change', updateHiddenFields);
  }
  if (indoorModuleIdSelect) {
    indoorModuleIdSelect.addEventListener('change', updateHiddenFields);
  }
  
  // Add event listener to form submission
  if (configForm) {
    configForm.addEventListener('submit', function(e) {
      // Update hidden fields before submission
      updateHiddenFields();
      
      console.log("Form submitted with values:", 
                  deviceIdActual ? deviceIdActual.value : 'N/A',
                  moduleIdActual ? moduleIdActual.value : 'N/A',
                  indoorModuleIdActual ? indoorModuleIdActual.value : 'N/A');
    });
  }
  
  // Add event listener to authorize button
  if (authorizeBtn) {
    authorizeBtn.addEventListener('click', function() {
      const clientId = document.getElementById('netatmoClientId').value;
      const clientSecret = document.getElementById('netatmoClientSecret').value;
      
      if (!clientId || !clientSecret) {
        alert("Please enter your Netatmo Client ID and Client Secret first.");
        return;
      }
      
      // Save the client ID and secret first
      const formData = new FormData();
      formData.append('netatmoClientId', clientId);
      formData.append('netatmoClientSecret', clientSecret);
      
      fetch('/save', {
        method: 'POST',
        body: formData
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          console.log("Credentials saved, initiating OAuth flow");
          
          // Now initiate the OAuth flow
          fetch('/api/netatmo/auth')
            .then(response => response.json())
            .then(data => {
              if (data.auth_url) {
                // Open the authorization URL in a new window
                window.open(data.auth_url, '_blank');
              } else if (data.error) {
                alert("Error: " + data.error);
              }
            })
            .catch(error => {
              console.error('Error initiating OAuth flow:', error);
              alert("Error initiating OAuth flow. Check console for details.");
            });
        } else {
          alert("Error saving credentials: " + (data.message || "Unknown error"));
        }
      })
      .catch(error => {
        console.error('Error saving credentials:', error);
        alert("Error saving credentials. Check console for details.");
      });
    });
  }
  
  // Check for OAuth callback parameters
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.has('oauth_success')) {
    alert("Successfully authorized with Netatmo! You can now select your devices.");
    fetchNetatmoDevices();
  } else if (urlParams.has('oauth_error')) {
    alert("Error during Netatmo authorization: " + urlParams.get('oauth_error'));
  }
  
  // Load config and populate fields immediately
  fetch('/config.json')
    .then(response => response.json())
    .then(data => {
      console.log("Config loaded, populating fields");
      
      // Populate manual fields with saved values
      if (deviceIdManual) deviceIdManual.value = data.netatmoDeviceId || '';
      if (moduleIdManual) moduleIdManual.value = data.netatmoModuleId || '';
      if (indoorModuleIdManual) indoorModuleIdManual.value = data.netatmoIndoorModuleId || '';
      
      // Update hidden fields
      updateHiddenFields();
      
      console.log("Manual fields populated:", 
                  deviceIdManual ? deviceIdManual.value : 'N/A',
                  moduleIdManual ? moduleIdManual.value : 'N/A',
                  indoorModuleIdManual ? indoorModuleIdManual.value : 'N/A');
      
      // If we have an access token, try to fetch devices
      if (data.netatmoAccessToken) {
        console.log("Access token found, fetching devices");
        fetchNetatmoDevices();
      } else {
        console.log("No access token found, skipping device fetch");
      }
    })
    .catch(err => {
      console.error('Failed to load config:', err);
    });
  
  // Handle device selection change
  if (deviceIdSelect) {
    deviceIdSelect.addEventListener('change', function() {
      // Clear module dropdowns
      moduleIdSelect.innerHTML = '<option value="">Select a module...</option>';
      indoorModuleIdSelect.innerHTML = '<option value="">Select a module...</option>';
      
      // Get selected device
      const selectedOption = this.options[this.selectedIndex];
      if (selectedOption && selectedOption.dataset.modules) {
        const modules = JSON.parse(selectedOption.dataset.modules);
        
        // Add modules to the dropdowns
        modules.forEach(module => {
          // Add to outdoor module dropdown
          const outdoorOption = document.createElement('option');
          outdoorOption.value = module.id;
          outdoorOption.textContent = `${module.name} (${module.type})`;
          moduleIdSelect.appendChild(outdoorOption);
          
          // Add to indoor module dropdown
          const indoorOption = document.createElement('option');
          indoorOption.value = module.id;
          indoorOption.textContent = `${module.name} (${module.type})`;
          indoorModuleIdSelect.appendChild(indoorOption);
        });
        
        // If we have saved module IDs, select them
        const savedModuleId = moduleIdManual.value;
        if (savedModuleId) {
          moduleIdSelect.value = savedModuleId;
        }
        
        const savedIndoorModuleId = indoorModuleIdManual.value;
        if (savedIndoorModuleId) {
          indoorModuleIdSelect.value = savedIndoorModuleId;
        }
      }
      
      // Update the manual input field
      deviceIdManual.value = this.value;
      
      // Update hidden fields
      updateHiddenFields();
    });
  }
  
  // Handle module selection changes
  if (moduleIdSelect) {
    moduleIdSelect.addEventListener('change', function() {
      moduleIdManual.value = this.value;
      updateHiddenFields();
    });
  }
  
  if (indoorModuleIdSelect) {
    indoorModuleIdSelect.addEventListener('change', function() {
      indoorModuleIdManual.value = this.value;
      updateHiddenFields();
    });
  }
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
        console.log("Received devices:", data.devices.length);
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
          console.log("Setting saved device ID:", savedDeviceId);
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
