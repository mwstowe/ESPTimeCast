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
  showStatus("Preparing to fetch Netatmo devices...", "loading");
  
  // First, check memory and defragment if needed
  fetch('/api/memory?defrag=true')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(memoryData => {
      console.log("Memory stats:", memoryData);
      showStatus(`Memory optimized. Free heap: ${memoryData.freeHeap} bytes. Testing connection...`, "loading");
      
      // Test connection to Netatmo API
      return fetch('/api/netatmo/test-connection');
    })
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(testResults => {
      console.log("Connection test results:", testResults);
      
      // Check if TLS connection works
      const tlsTest = testResults.tests.find(test => test.name === "TLS Connection");
      const httpTest = testResults.tests.find(test => test.name === "HTTP GET");
      
      if (tlsTest && !tlsTest.success) {
        showStatus("TLS connection to Netatmo API failed. Using mock data instead.", "warning");
        console.log("Using mock data due to TLS connection failure");
        return fetch('/api/netatmo/mock-devices');
      }
      
      if (httpTest && !httpTest.success) {
        showStatus("HTTP connection to Netatmo API failed. Using mock data instead.", "warning");
        console.log("Using mock data due to HTTP connection failure");
        return fetch('/api/netatmo/mock-devices');
      }
      
      // Now check token status
      return fetch('/api/netatmo/token-status');
    })
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(status => {
      console.log("Token status:", status);
      
      if (!status.hasAccessToken) {
        showStatus("No access token available. Please authorize Netatmo first.", "error");
        throw new Error("No access token available");
      }
      
      // Step 1: Trigger the device fetch to save to file
      return fetch('/api/netatmo/fetch-devices', { method: 'POST' });
    })
    .then(response => {
      if (!response.ok) {
        return response.text().then(text => {
          try {
            const errorObj = JSON.parse(text);
            throw new Error(`API error: ${errorObj.error || response.statusText}`);
          } catch (e) {
            throw new Error(`API error: ${response.status} ${response.statusText}`);
          }
        });
      }
      return response.json();
    })
    .then(data => {
      console.log("Device fetch initiated:", data);
      
      if (data.status === "success") {
        showStatus("Devices saved to file. Loading device data...", "loading");
        
        // Step 2: Retrieve the saved device data
        return fetch('/api/netatmo/saved-devices');
      } else if (data.status === "token_refreshed") {
        showStatus("Token refreshed. Please try again.", "warning");
        setTimeout(() => {
          hideStatus();
        }, 3000);
        throw new Error("Token refreshed. Please try again.");
      } else if (data.status === "initiated") {
        // The fetch was initiated but not completed yet
        showStatus("Device fetch initiated. Please wait...", "loading");
        
        // Poll for completion
        return new Promise((resolve, reject) => {
          let attempts = 0;
          const maxAttempts = 10;
          
          const checkStatus = () => {
            attempts++;
            console.log(`Checking for device data (attempt ${attempts}/${maxAttempts})...`);
            
            fetch('/api/netatmo/saved-devices')
              .then(response => {
                if (response.ok) {
                  resolve(response.json());
                } else if (attempts < maxAttempts) {
                  // Wait and try again
                  setTimeout(checkStatus, 2000);
                } else {
                  // Try mock data as a last resort
                  console.log("Falling back to mock data after failed attempts");
                  return fetch('/api/netatmo/mock-devices')
                    .then(mockResponse => {
                      if (mockResponse.ok) {
                        showStatus("Using mock data after failed API attempts", "warning");
                        resolve(mockResponse.json());
                      } else {
                        reject(new Error("Timed out waiting for device data and mock data unavailable"));
                      }
                    })
                    .catch(error => {
                      reject(error);
                    });
                }
              })
              .catch(error => {
                if (attempts < maxAttempts) {
                  // Wait and try again
                  setTimeout(checkStatus, 2000);
                } else {
                  // Try mock data as a last resort
                  console.log("Falling back to mock data after error");
                  fetch('/api/netatmo/mock-devices')
                    .then(mockResponse => {
                      if (mockResponse.ok) {
                        showStatus("Using mock data after API errors", "warning");
                        resolve(mockResponse.json());
                      } else {
                        reject(error);
                      }
                    })
                    .catch(mockError => {
                      reject(error);
                    });
                }
              });
          };
          
          // Start polling after a short delay
          setTimeout(checkStatus, 2000);
        });
      } else {
        throw new Error(data.message || "Unknown error");
      }
    })
    .catch(error => {
      console.error(`Error in fetch chain: ${error.message}`);
      
      // Try to use mock data as fallback
      showStatus(`API error: ${error.message}. Trying mock data...`, "warning");
      
      return fetch('/api/netatmo/mock-devices')
        .then(response => {
          if (!response.ok) {
            throw new Error("Mock data not available");
          }
          return response.json();
        });
    })
    .then(data => {
      console.log("Processing device data");
      
      // Process the device data
      let devices = [];
      
      if (data.body && data.body.devices) {
        console.log("Using standard format (body.devices)");
        devices = data.body.devices;
      } else if (data.devices) {
        console.log("Using alternative format (devices)");
        devices = data.devices;
      } else {
        console.error("Unexpected data format");
        showStatus("Unexpected data format from API", "error");
        return;
      }
      
      if (!devices || devices.length === 0) {
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
      console.error(`Final error: ${error.message}`);
      showStatus(`Failed to fetch Netatmo devices: ${error.message}`, "error");
    });
}
// Function to connect to Netatmo
function connectNetatmo() {
  console.log("Connecting to Netatmo...");
  
  // Get the client ID and client secret
  const clientId = document.getElementById('netatmoClientId').value;
  const clientSecret = document.getElementById('netatmoClientSecret').value;
  
  if (!clientId || !clientSecret) {
    showStatus("Please enter your Netatmo Client ID and Client Secret", "error");
    return;
  }
  
  // Show loading message
  showStatus("Connecting to Netatmo...", "loading");
  
  // Save the credentials
  fetch('/api/netatmo/save-credentials', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: `clientId=${encodeURIComponent(clientId)}&clientSecret=${encodeURIComponent(clientSecret)}`
  })
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.json();
  })
  .then(data => {
    console.log("Credentials saved:", data);
    
    // Redirect to the auth endpoint
    window.location.href = '/api/netatmo/auth';
  })
  .catch(error => {
    console.error(`Error saving credentials: ${error.message}`);
    showStatus(`Failed to save credentials: ${error.message}`, "error");
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
// Function to show status message
function showStatus(message, type) {
  let statusDiv = document.getElementById('statusMessage');
  
  if (!statusDiv) {
    statusDiv = document.createElement('div');
    statusDiv.id = 'statusMessage';
    const container = document.querySelector('.container');
    const h1 = document.querySelector('h1');
    
    if (container && h1) {
      container.insertBefore(statusDiv, h1.nextElementSibling);
    } else {
      document.body.insertBefore(statusDiv, document.body.firstChild);
    }
  }
  
  statusDiv.textContent = message;
  statusDiv.className = 'status ' + type;
  statusDiv.style.display = 'block';
}

// Function to hide status message
function hideStatus() {
  const statusDiv = document.getElementById('statusMessage');
  if (statusDiv) {
    statusDiv.style.display = 'none';
  }
}

// Function to update token status
function updateTokenStatus(accessToken) {
  const tokenStatus = document.getElementById('tokenStatus');
  if (!tokenStatus) return;
  
  if (accessToken && accessToken.length > 0) {
    tokenStatus.textContent = 'Connected to Netatmo';
    tokenStatus.className = 'token-status token-valid';
    
    const connectButton = document.getElementById('connectButton');
    if (connectButton) {
      connectButton.textContent = 'Reconnect to Netatmo';
    }
  } else {
    tokenStatus.textContent = 'Not connected to Netatmo';
    tokenStatus.className = 'token-status token-expired';
  }
}
// Function to load API keys
function loadApiKeys() {
  console.log("Loading API keys...");
  
  fetch('/api/netatmo/get-credentials')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("API keys loaded:", data);
      
      // Update the input fields
      const clientIdInput = document.getElementById('netatmoClientId');
      if (clientIdInput && data.clientId) {
        clientIdInput.value = data.clientId;
        clientIdInput.placeholder = "Your Netatmo API Client ID";
      }
      
      // Update the client secret field
      const clientSecretInput = document.getElementById('netatmoClientSecret');
      if (clientSecretInput && data.clientSecret) {
        clientSecretInput.value = data.clientSecret;
        clientSecretInput.placeholder = "Your Netatmo API Client Secret";
      }
    })
    .catch(error => {
      console.error(`Error loading API keys: ${error.message}`);
    });
}

// Initialize the page when it loads
document.addEventListener('DOMContentLoaded', function() {
  console.log("Netatmo devices page loaded");
  
  // Load API keys
  loadApiKeys();
  
  // Check if we have an access token
  fetch('/api/netatmo/token-status')
    .then(response => {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error("Not authenticated with Netatmo");
      }
    })
    .then(data => {
      if (data.hasAccessToken) {
        console.log("Found access token");
        updateTokenStatus("valid-token");
      }
    })
    .catch(error => {
      console.log("No access token found:", error.message);
      updateTokenStatus(null);
    });
  
  // Set up form submission
  const form = document.getElementById('netatmoForm');
  if (form) {
    form.addEventListener('submit', function(event) {
      event.preventDefault();
      
      const deviceId = document.getElementById('netatmoDeviceId').value;
      const moduleId = document.getElementById('netatmoModuleId').value;
      const indoorModuleId = document.getElementById('netatmoIndoorModuleId').value;
      
      // Show loading message
      showStatus("Saving settings...", "loading");
      
      // Save the settings
      fetch('/api/netatmo/save-settings', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `netatmoDeviceId=${encodeURIComponent(deviceId)}&netatmoModuleId=${encodeURIComponent(moduleId)}&netatmoIndoorModuleId=${encodeURIComponent(indoorModuleId)}`
      })
      .then(response => {
        if (!response.ok) {
          throw new Error(`HTTP error! Status: ${response.status}`);
        }
        return response.json();
      })
      .then(data => {
        console.log("Settings saved:", data);
        showStatus("Settings saved successfully", "success");
        setTimeout(hideStatus, 3000);
      })
      .catch(error => {
        console.error(`Error saving settings: ${error.message}`);
        showStatus(`Failed to save settings: ${error.message}`, "error");
      });
    });
  }
});
// Function to initialize mock data
function initMockData() {
  console.log("Initializing mock data...");
  showStatus("Initializing mock data...", "loading");
  
  fetch('/api/netatmo/init-mock-data')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Mock data initialized:", data);
      showStatus("Mock data initialized. Loading devices...", "success");
      
      // Now load the mock data
      return fetch('/api/netatmo/saved-devices');
    })
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Loaded mock device data");
      
      // Process the device data
      let devices = [];
      
      if (data.body && data.body.devices) {
        console.log("Using standard format (body.devices)");
        devices = data.body.devices;
      } else if (data.devices) {
        console.log("Using alternative format (devices)");
        devices = data.devices;
      } else {
        console.error("Unexpected data format");
        showStatus("Unexpected data format from mock data", "error");
        return;
      }
      
      if (!devices || devices.length === 0) {
        console.log("No devices found in mock data");
        showStatus("No devices found in mock data", "error");
        return;
      }
      
      // Store devices in global variable
      window.netatmoDevices = devices;
      
      // Get the device select element
      const deviceSelect = document.getElementById('netatmoDeviceId');
      const moduleSelect = document.getElementById('netatmoModuleId');
      const indoorModuleSelect = document.getElementById('netatmoIndoorModuleId');
      
      if (!deviceSelect || !moduleSelect || !indoorModuleSelect) {
        console.error("Device or module select elements not found");
        return;
      }
      
      // Clear existing options
      deviceSelect.innerHTML = '<option value="">Select Device...</option>';
      moduleSelect.innerHTML = '<option value="none">Not mapped</option>';
      indoorModuleSelect.innerHTML = '<option value="none">Not mapped</option>';
      
      // Populate device dropdown
      devices.forEach(device => {
        const option = document.createElement('option');
        option.value = device._id || device.id;
        option.textContent = device.station_name || device.name || device._id || device.id;
        deviceSelect.appendChild(option);
      });
      
      console.log(`Found ${devices.length} mock devices`);
      showStatus(`Found ${devices.length} mock devices`, "success");
      setTimeout(hideStatus, 3000);
    })
    .catch(error => {
      console.error(`Error initializing mock data: ${error.message}`);
      showStatus(`Failed to initialize mock data: ${error.message}`, "error");
    });
}
