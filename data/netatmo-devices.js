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
  
  // First, check if the ESP has the data or if we need to fetch directly
  fetch('/api/netatmo/devices?refresh=true')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      // Check if we need to make a direct API call
      if (data.redirect && data.redirect === "api") {
        console.log("ESP requested direct API call");
        
        // Get the access token from the ESP
        return fetch('/api/netatmo/token')
          .then(response => {
            if (!response.ok) {
              throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
          })
          .then(tokenData => {
            if (!tokenData.access_token) {
              throw new Error("No access token available");
            }
            
            console.log("Got access token from ESP");
            
            // Make the API call directly from the browser
            return fetch('https://api.netatmo.com/api/homesdata', {
              headers: {
                'Authorization': `Bearer ${tokenData.access_token}`,
                'Accept': 'application/json'
              }
            });
          })
          .then(response => {
            if (!response.ok) {
              throw new Error(`Netatmo API error! Status: ${response.status}`);
            }
            return response.json();
          });
      } else {
        // ESP already has the data
        console.log("Using data from ESP");
        return data;
      }
    })
    .then(data => {
      console.log("Received device data from Netatmo API:", data);
      
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
