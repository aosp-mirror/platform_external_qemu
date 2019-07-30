// Googleplex!
var lastLatLng = {lat: 37.4220919, lng: -122.0826088};
var sevDeviceLocation;
var gMap;
var gGeocoder;
var gCurrentMarker; // The blue dot
var gPendingMarker; // The red "pin"
var gSearchResultMarkers = [];

function savePoint() {
    // The emulator should have the point of interest already, from the
    // sendAddress() call.
    channel.objects.emulocationserver.map_savePoint();
}

function hideAllOverlays() {
    document.getElementById("savePointOverlay").style.display = "none";
    document.getElementById("pointInfoOverlay").style.display = "none";
}

function resetPointsMap() {
    hideAllOverlays();
}

function showPointInfoOverlay(address, latLng, elevation) {
    hideAllOverlays();
    document.getElementById("pointInfoTitle").innerHTML = address;
    var lat = latLng.lat().toFixed(4);
    var lng = latLng.lng().toFixed(4);
    subtitle = lat + ", " + lng + (elevation != null ? ", " + elevation.toFixed(4) : "");
    document.getElementById("pointInfoSubtitle").innerHTML = subtitle;
    document.getElementById("pointInfoOverlay").style.display = "flex";
}

// Callback function for Maps API
function initMap() {
    // Create a map object and specify the DOM element for display.
    gMap = new google.maps.Map(document.getElementById('map'), {
        center: lastLatLng,
        zoom: 10,
        zoomControl: true,
        disableDefaultUI: true
    });
    gGeocoder = new google.maps.Geocoder;

    function showPin(latLng) {
        // Clear current pin
        if (gPendingMarker != null) {
            gPendingMarker.setMap(null);
        }
        // Clear all current search markers
        for (var i = 0, marker; marker = gSearchResultMarkers[i]; i++) {
            marker.setMap(null);
        }
        gSearchResultMarkers = [];

        // Create a new pin at this location
        gPendingMarker = new google.maps.Marker({
            map: gMap,
            position: latLng
        });
        var bounds = gMap.getBounds();
        if (bounds == null || !bounds.contains(latLng)) {
            gMap.setCenter(latLng);
        }
    }
    function showSavePointOverlay(address, latLng, elevation) {
        hideAllOverlays();
        document.getElementById("savePointTitle").innerHTML = address;
        var lat = latLng.lat().toFixed(4);
        var lng = latLng.lng().toFixed(4);
        subtitle = lat + ", " + lng + (elevation != null ? ", " + elevation.toFixed(4) : "");
        document.getElementById("savePointSubtitle").innerHTML = subtitle;
        document.getElementById("savePointOverlay").style.display = "block";
    }
    function sendAddress(latLng) {
        gGeocoder.geocode({ 'location': latLng }, function(results, status) {
            var address = "";
            var elevation = 0.0;
            if (status === 'OK' && results[0]) {
              address = results[0].formatted_address;
              elevation = results[0].elevation;
            }
            showSavePointOverlay(address, latLng, elevation);
            channel.objects.emulocationserver
                .sendLocation(latLng.lat(), latLng.lng(), address);
            console.log("addr=" + address);
            document.getElementById('pac-input').value = address;
            document.getElementById('search-icon').style.display = 'block';
            document.getElementById('search-spinner').style.display = 'none';
        });
    }

    // Register a listener that sets a new marker wherever the
    // user clicks on the map.
    google.maps.event.addListener(gMap, 'click', function(event) {
        // TODO: add spinner here
        document.getElementById('search-icon').style.display = 'none';
        document.getElementById('search-spinner').style.display = 'flex';
        showPin(event.latLng);

        // Send the location coordinates to the emulator
        lastLatLng = event.latLng;
        sendAddress(lastLatLng);
    });

    showPendingLocation = function(lat, lng, addr) {
        // Called from Qt code to show a pending location on the UI map.
        // This point has NOT been sent to the AVD.
        var latLng = new google.maps.LatLng(lat, lng);
        showPin(latLng);
        // TODO: no support for elevation yet.
        showPointInfoOverlay(addr, latLng, null);
    }

    setDeviceLocation = function(lat, lng) {
        // Called from Qt code to show the blue marker to display the emulator location on the map.
        // Code to set location marker on the map. Move this to a signal event handler.

        // Clear any existing blue dot and red pin
        if (gCurrentMarker != null) {
            gCurrentMarker.setMap(null);
        }
        if (gPendingMarker != null) {
            gPendingMarker.setMap(null);
        }

        var latLng = new google.maps.LatLng(lat, lng);
        gCurrentMarker = new google.maps.Marker({ map: gMap, position: latLng});

        var blueDot = 'data:image/svg+xml, \
                <svg xmlns="http://www.w3.org/2000/svg" width="24px" height="24px" \
                        viewBox="0 0 24 24" fill="%23000000"> \
                    <circle cx="12" cy="12" r="10" fill="black" opacity=".1" /> \
                    <circle cx="12" cy="12" r="8" stroke="white" stroke-width="2" \
                        fill="%234286f5" /> \
                </svg>';
        var image = {
          url: blueDot,
          size: new google.maps.Size(24, 24),
          origin: new google.maps.Point(0, 0),
          anchor: new google.maps.Point(12, 12),
          scaledSize: new google.maps.Size(25, 25)
        };
        var bounds = gMap.getBounds();
        if (bounds == null || !bounds.contains(latLng)) {
            gMap.setCenter(latLng);
        }
        gCurrentMarker.setIcon(image);
    }

    function markerListener(event) {
        lastLatLng = event.latLng;
        showPin(lastLatLng);
        sendAddress(lastLatLng);
    }

    // Add Google search box
    // Create the search box and link it to the UI element.
    var searchContainer = document.getElementById('search-container');
    var input = /** @type {HTMLInputElement} */ (
            document.getElementById('pac-input'));
    gMap.controls[google.maps.ControlPosition.TOP_CENTER].push(searchContainer);

    var autocomplete = new google.maps.places.Autocomplete(
            /** @type {HTMLInputElement} */
            (input));
    autocomplete.bindTo('bounds', gMap);
    // Set the data fields to return when the user selects a place.
    autocomplete.setFields(
        ['address_components', 'geometry', 'icon', 'name']);

    // Listen for the event fired when the user selects an item from the
    // pick list.
    autocomplete.addListener('place_changed', function() {
        // Clear the red pin and any existing search markers
        if (gPendingMarker != null) {
            gPendingMarker.setMap(null);
        }
        for (var i = 0, marker; marker = gSearchResultMarkers[i]; i++) {
            marker.setMap(null);
        }
        gSearchResultMarkers = [];

        var place = autocomplete.getPlace();
        if (!place.geometry) {
            // User entered the name of a Place that was not suggested and
            // pressed the Enter key, or the Place Details request failed.
            // Just ignore it.
            return;
        }

        lastLatLng = place.geometry.location;
        showPendingLocation(lastLatLng.lat(), lastLatLng.lng());
        sendAddress(lastLatLng);

        // If the place has a geometry, then present it on a map.
        if (place.geometry.viewport) {
            gMap.fitBounds(place.geometry.viewport);
        } else {
            gMap.setCenter(place.geometry.location);
            gMap.setZoom(17);  // Why 17? Because it looks good.
        }
    });

    document.getElementById('search-icon').addEventListener('click', function() {
        if (input.value.length > 0) {
            input.value = "";
            hideAllOverlays();
            if (gPendingMarker != null) {
                gPendingMarker.setMap(null);
            }
        }
    });
}

