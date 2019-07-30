// Googleplex!
var lastLatLng = {lat: 37.4220919, lng: -122.0826088};
var gMap;
var gFirstPoint;
// TODO: replace the gstart and gend with WaypointInfo classes
var gStartLatLng;
var gEndLatLng;
var gStartWaypoint;
var gEndWaypoint;
var gFocusedWaypoint;
// TODO: add intermediate waypoints
var gHaveActiveRoute;
var gMarkers = [];
var gDirectionsService;
var gDirectionsDisplay;
var gEndMarker;
var gTravelModeString = "DRIVING";
var gGeocoder;
var gAutocompleteService;
var gLastFocusedAddrBox;

class WaypointInfo {
    constructor() {
        this.latLng = null;
        this.address = null;
        this.marker = null;
    }

    setAddress(address) {
        this.address = address;
    }

    setLatLng(latLng) {
        this.latLng = latLng;
    }

    setMarker(marker) {
        this.marker = marker;
    }

    setInputDiv(div) {
        this.inputDiv = div;
    }

    isValid() {
        return (this.latLng != null);
    }

    invalidate() {
        this.latLng = null;
        this.address = "";
        if (this.marker != null) {
            this.marker.setMap(null);
            this.marker = null;
        }
    }
}

function showSearchSpinner(visible) {
    if (visible) {
        document.getElementById('search-icon').style.display = 'none';
        document.getElementById('search-spinner').style.display = 'flex';
    } else {
        document.getElementById('search-icon').style.display = 'block';
        document.getElementById('search-spinner').style.display = 'none';
    }
}

function hideAllOverlays() {
    document.getElementById("startInfoOverlay").style.display = "none";
    document.getElementById("routeCreatorOverlay").style.display = "none";
}

function showStartInfoOverlay(address, latLng, elevation) {
    hideAllOverlays();

    document.getElementById("startInfoTitle").innerHTML = address;
    var lat = latLng.lat().toFixed(4);
    var lng = latLng.lng().toFixed(4);
    subtitle = lat + ", " + lng + (elevation != null ? ", " + elevation.toFixed(4) : "");
    document.getElementById("startInfoSubtitle").innerHTML = subtitle;
    document.getElementById("startInfoOverlay").style.display = "flex";
}

function showRouteCreatorOverlay(isSavedRoute) {
    document.getElementById("routeCreatorOverlay").style.display = "flex";
}

function inRouteCreatorMode() {
    let disp = document.getElementById('routeCreatorOverlay').style.display;
    return (disp.length > 0 && disp !== "none");
}

function getTransportMode() {
    return document.querySelector('input[name="transportMode"]:checked').value;
}

function setTransportMode(mode) {
    var modes = document.getElementsByName('transportMode');
    for (var i = 0; i < modes.length; ++i) {
        if (mode === modes[i].value) {
            modes[i].checked = true;
            return;
        }
    }
}

function resetRouteCreatorOverlay() {
    setTransportMode("DRIVING");
    document.getElementById('startAddress').value = '';
    document.getElementById('endAddress').value = '';
    document.getElementById('routeCreatorOverlay').style.height = 'auto';
    document.getElementById('searchResultsOverlay').style.display = 'none';
    gStartWaypoint.invalidate();
    gEndWaypoint.invalidate();
    clearDirections();
}

function clearSearchResults() {
    var searchContainer = document.getElementById("searchResultsOverlay");
    while (searchContainer.firstChild) {
        searchContainer.removeChild(searchContainer.firstChild);
    }
}

function addSearchResultItem(name, address, placeId) {
    var searchItem = document.createElement('div');
    searchItem.classList.add("search-item");

    var icon = document.createElement('div');
    icon.classList.add("search-item-icon");
    icon.classList.add("maps-sprite-suggest-place-pin");
    icon.classList.add("grey-marker-icon");
    searchItem.appendChild(icon);

    var nameSpan = document.createElement('span');
    nameSpan.classList.add("name");
    nameSpan.innerHTML = name + " ";
    searchItem.appendChild(nameSpan);

    var addressSpan = document.createElement('span');
    addressSpan.classList.add("address");
    addressSpan.innerHTML = address;
    nameSpan.appendChild(addressSpan);

    var searchContainer = document.getElementById("searchResultsOverlay");
    searchContainer.appendChild(searchItem);

    searchItem.setAttribute("data-place-id", placeId);
    searchItem.addEventListener('click', function() {
        var placeid = this.getAttribute("data-place-id");
        var address = this.getAttribute("data-address");
        if (placeid != null) {
            gGeocoder.geocode({ 'placeId': placeid }, function(results, status) {
                if (status !== 'OK' || !results[0]) {
                    return;
                }
                document.getElementById('searchResultsOverlay').style.display = 'none';
                clearSearchResults();
                document.getElementById('routeCreatorOverlay').style.height = 'auto';
                console.log("latlng=" + results[0].geometry.location);
                if (setWaypointForActiveAddressBox(results[0].geometry.location,
                                                   results[0].formatted_address)) {
                    calcRoute();
                }
            });
        }
    });
}

function isAllWaypointsValid() {
    // TODO: numWaypoints will change once intermediate waypoints are added.
    var waypoints = [gStartWaypoint, gEndWaypoint];
    for (var i = 0; i < waypoints.length; ++i) {
        if (!waypoints[i].isValid()) {
            return false;
        }
    }
    return true;
}

function setWaypointForActiveAddressBox(latLng, address) {
    var focusedWaypoint;
    if (typeof(gLastFocusedAddrBox) === "undefined") {
        console.log("addr box null");
        return false;
    }

    if (gLastFocusedAddrBox.id === "startAddress") {
        focusedWaypoint = gStartWaypoint;
        console.log("start");
    } else if (gLastFocusedAddrBox.id === "endAddress") {
        focusedWaypoint = gEndWaypoint;
        console.log("end");
    } else {
        // TODO: intermediate waypoint
        console.log("intermediate");
    }

    // Clear current pin
    focusedWaypoint.invalidate();

    focusedWaypoint.setLatLng(latLng);
    focusedWaypoint.setAddress(address);
    gLastFocusedAddrBox.value = address;
    return isAllWaypointsValid();
}

function setWaypointForEmptyAddressBox(latLng) {
    // Just set the waypoint for the first empty input box we come across.
    var waypointInputs = document.querySelectorAll('.waypoint-addr-input');
    var foundEmptyInput = false;
    var numEmptyInputs = 0;
    for (var i = 0; i < waypointInputs.length; ++i) {
        if (waypointInputs[i].value === "") {
            if (!foundEmptyInput) {
                focusedWaypointInput = waypointInputs[i];
                foundEmptyInput = true;
            }
            // track the number of empty inputs. return true if we only have
            // one empty input remaining.
            numEmptyInputs++;
        }
    }
    if (!foundEmptyInput) {
        return false;
    }

    if (focusedWaypointInput.id === "startAddress") {
        gFocusedWaypoint = gStartWaypoint;
    } else if (focusedWaypointInput.id === "endAddress") {
        gFocusedWaypoint = gEndWaypoint;
    } else {
        // TODO: intermediate waypoint
    }

    // Clear current pin
    gFocusedWaypoint.invalidate();

    gFocusedWaypoint.setLatLng(latLng);
    gGeocoder.geocode({ 'location': latLng }, function(results, status) {
        var address = "";
        var elevation = 0.0;
        if (status === 'OK' && results[0]) {
          address = results[0].formatted_address;
          elevation = results[0].elevation;
        }
        gFocusedWaypoint.setAddress(address);
        gFocusedWaypoint.inputDiv.value = address;
    });

    return isAllWaypointsValid();
}

function clearDirections() {
    // Remove the old path
    gHaveActiveRoute = false;
    if (gDirectionsDisplay != null) {
        gDirectionsDisplay.setMap(null);
        gDirectionsDisplay = null;
    }
    document.getElementById('saveRouteButton').style.display = "none";
}

function onHaveActiveRoute() {
    gHaveActiveRoute = true;
    document.getElementById('saveRouteButton').style.display = "flex";
}

function saveRouteButton_clicked() {
    document.getElementById('saveRouteButton').style.display = "none";
    // emulator already has the route metadata. Just tell emulator to save it.
    channel.objects.emulocationserver.saveRoute();
}

function onReverseButtonClicked() {
    var startAddrDiv = document.getElementById('startAddress');
    var endAddrDiv = document.getElementById('endAddress');
    var tmpStr = startAddrDiv.value;
    startAddrDiv.value = endAddrDiv.value;
    endAddrDiv.value = tmpStr;

    var tmpWaypoint = gStartWaypoint;
    gStartWaypoint = gEndWaypoint;
    gEndWaypoint = tmpWaypoint;

    var tmpInput = gStartWaypoint.inputDiv;
    gStartWaypoint.setInputDiv(gEndWaypoint.inputDiv);
    gEndWaypoint.setInputDiv(tmpInput);

    calcRoute();
}

// Callback function for Maps API
function initMap() {
    var infoWindow = new google.maps.InfoWindow;
    // Create a map object and specify the DOM element for display.
    gMap = new google.maps.Map(document.getElementById('map'), {
        center: lastLatLng,
        zoom: 10,
        zoomControl: true,
        disableDefaultUI: true
    });

    gGeocoder = new google.maps.Geocoder;
    gFirstPoint = true;
    clearDirections();

    gStartWaypoint = new WaypointInfo;
    gStartWaypoint.setInputDiv(document.getElementById('startAddress'));
    gEndWaypoint = new WaypointInfo;
    gEndWaypoint.setInputDiv(document.getElementById('endAddress'));

    gBlueMarker = new google.maps.Marker({map: null, position: lastLatLng});

    // Register a listener that sets a new marker wherever user clicks
    // on the map.
    google.maps.event.addListener(gMap, 'click', function(event) {
        if (inRouteCreatorMode()) {
            if (setWaypointForEmptyAddressBox(event.latLng)) {
                calcRoute();
            }
        } else {
            showDestinationPoint(event.latLng);
        }
    });

    function showDestinationPoint(latLng) {
        showSearchSpinner(true);
        gEndLatLng = latLng;

        // Clear current pin
        gEndWaypoint.invalidate();

        gEndWaypoint.setLatLng(latLng);
        gEndWaypoint.setMarker(new google.maps.Marker({
            map: gMap,
            position: latLng
        }));
        // Remove the old path
        clearDirections();

        // Ensure that the starting point is visible
        if (!gMap.getBounds().contains(latLng)) {
            gMap.setCenter(latLng);
        }
        // Give the Emulator an empty route, so it knows we've
        // got something new under way.
        channel.objects.emulocationserver.sendFullRouteToEmu(0, 0.0, null, null);

        gGeocoder.geocode({ 'location': latLng }, function(results, status) {
            var address = "";
            var elevation = 0.0;
            if (status === 'OK' && results[0]) {
              address = results[0].formatted_address;
              elevation = results[0].elevation;
            }
            gEndWaypoint.setAddress(address);
            showStartInfoOverlay(address, latLng, elevation);
            document.getElementById('pac-input').value = address;
            showSearchSpinner(false);
        });
    }

    setDeviceLocation = function(lat, lng) {
        // Called from Qt code to show the blue marker to display the emulator location on the map.

        var latLng = new google.maps.LatLng(lat, lng);
        gBlueMarker.setMap(null); // Remove the old marker
        gBlueMarker = new google.maps.Marker({map: gMap, position: latLng});

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
        gBlueMarker.setIcon(image);
        gMap.setCenter(latLng);
    }

    // Receive the selected route from the host
    setRouteOnMap = function(routeJson) {
        if (routeJson.length > 0) {
            hideAllOverlays();
            resetRouteCreatorOverlay();
            var theRoute = JSON.parse(routeJson);
            if (gDirectionsDisplay != null) {
                // Clear any previous route
                gDirectionsDisplay.setMap(null);
            }

            gDirectionsDisplay = new google.maps.DirectionsRenderer();
            gDirectionsDisplay.setMap(gMap);
            gDirectionsDisplay.setDirections(theRoute);

            gStartWaypoint.setLatLng(theRoute['request']['origin']['location']);
            gEndWaypoint.setLatLng(theRoute['request']['destination']['location']);
            gTravelModeString = theRoute['request']['travelMode'];

            document.getElementById('startAddress').value = theRoute.routes[0].legs[0].start_address;
            document.getElementById('endAddress').value = theRoute.routes[0].legs[0].end_address;
            showRouteCreatorOverlay(true);
        }
        gInputBox['value'] = '';
    }

    // Add Google search box
    // Create the search box and link it to the UI element.
    var searchContainer = document.getElementById('search-container');
    var gInputBox = /** @type {HTMLInputElement} */ (
            document.getElementById('pac-input'));
    gMap.controls[google.maps.ControlPosition.TOP_CENTER].push(searchContainer);

    var autocomplete = new google.maps.places.Autocomplete(
            /** @type {HTMLInputElement} */
            (gInputBox));
    autocomplete.bindTo('bounds', gMap);

    // For the route creator
    gAutocompleteService = new google.maps.places.AutocompleteService();

    // Listen for the event fired when the user selects an item from the
    // pick list.
    autocomplete.addListener('place_changed', function() {
        // Clear any existing markers
        for (var i = 0, marker; marker = gMarkers[i]; i++) {
            marker.setMap(null);
        }
        gMarkers = [];

        var place = autocomplete.getPlace();
        if (!place.geometry) {
            // User entered the name of a Place that was not suggested and
            // pressed the Enter key, or the Place Details request failed.
            // Just ignore it.
            return;
        }

        // If the place has a geometry, then present it on a map.
        if (place.geometry.viewport) {
            gMap.fitBounds(place.geometry.viewport);
        } else {
            gMap.setCenter(place.geometry.location);
            gMap.setZoom(17);  // Why 17? Because it looks good.
        }

        showDestinationPoint(place.geometry.location);
    });

    document.getElementById('search-icon').addEventListener('click', function() {
        if (gInputBox.value.length > 0) {
            gInputBox.value = "";
            hideAllOverlays();
        }
    });
    document.getElementById('closeRouteCreatorButton').addEventListener('click', function() {
        if (gDirectionsDisplay != null) {
            gDirectionsDisplay.setMap(null);
            gDirectionsDisplay = null;
        }
        document.getElementById("routeCreatorOverlay").style.display = "none";
        document.getElementById("startInfoOverlay").style.display = "flex";
        resetRouteCreatorOverlay();
    });

    document.getElementById('startNavButton').addEventListener('click', function() {
        hideAllOverlays();
        document.getElementById('endAddress').value = gEndWaypoint.address;
        document.getElementById('startAddress').value = "";
        showRouteCreatorOverlay(false);
    });

    var displaySuggestions = function(predictions, status) {
        if (status != google.maps.places.PlacesServiceStatus.OK) {
            return;
        }

        predictions.forEach(function(prediction) {
            addSearchResultItem(prediction.structured_formatting.main_text,
                                prediction.structured_formatting.secondary_text,
                                prediction.place_id);
        });
    };

    document.getElementById('startAddress').addEventListener('input', function() {
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        clearDirections();
        gStartWaypoint.invalidate();
        if (this.value === "") {
            document.getElementById('searchResultsOverlay').style.display = 'none';
            overlay.style.height = "auto";
        } else {
            overlay.style.height = "100%";
            document.getElementById('searchResultsOverlay').style.display = 'block';
            gLastFocusedAddrBox = this;
            gAutocompleteService.getPlacePredictions({input: this.value}, displaySuggestions);
        }
    });

    document.getElementById('startAddress').addEventListener('focusin', function() {
        this.select();
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        document.getElementById('searchResultsOverlay').style.display = 'none';
        overlay.style.height = "auto";
    });

    document.getElementById('endAddress').addEventListener('focusin', function() {
        this.select();
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        document.getElementById('searchResultsOverlay').style.display = 'none';
        overlay.style.height = "auto";
    });

    document.getElementById('endAddress').addEventListener('input', function() {
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        clearDirections();
        gEndWaypoint.invalidate();
        if (this.value === "") {
            document.getElementById('searchResultsOverlay').style.display = 'none';
            overlay.style.height = "auto";
        } else {
            gLastFocusedAddrBox = this;
            overlay.style.height = "100%";
            document.getElementById('searchResultsOverlay').style.display = 'block';
            gAutocompleteService.getQueryPredictions({input: this.value}, displaySuggestions);
        }
    });

    var modes = document.getElementsByName('transportMode');
    for (var i = 0; i < modes.length; ++i) {
        modes[i].addEventListener('change', function() {
           if (this.checked) {
               console.log("Setting transport mode to " + this.value);
               gTravelModeString = this.value;
               calcRoute();
           }
        });
    }

    // Set up to retrieve directions
    gDirectionsService = new google.maps.DirectionsService();
}

// TODO: add parameter to this function to accept multiple waypoints.
function calcRoute() {
    if (!gStartWaypoint.isValid() || !gEndWaypoint.isValid()) {
        return;
    }
    clearDirections();

    var request = {
        origin: gStartWaypoint.latLng,
        destination: gEndWaypoint.latLng,
        travelMode: getTransportMode()
    };
    gDirectionsDisplay = new google.maps.DirectionsRenderer();
    gDirectionsDisplay.setMap(gMap);
    gDirectionsService.route(request, function(result, status) {
        if (status != 'OK') {
            return;
        }
        gDirectionsDisplay.setDirections(result);
        onHaveActiveRoute();

        // Calculate the number of points and the total duration of the route
        var numPoints = 0;
        var totalDuration = 0.0;
        for (var legIdx = 0;
             legIdx < result.routes[0].legs.length;
             legIdx++)
        {
            totalDuration += result.routes[0].legs[legIdx].duration.value;

            var numSteps = result.routes[0].legs[legIdx].steps.length;
            for (var stepIdx = 0; stepIdx < numSteps; stepIdx++) {
                numPoints += result.routes[0].legs[legIdx].steps[stepIdx].path.length;
            }
            numPoints -= (numSteps - 1); // Don't count the duplicate first points
        }

        var fullResult = JSON.stringify(result);
        channel.objects.emulocationserver.sendFullRouteToEmu(numPoints, totalDuration, fullResult, gTravelModeString);
    });
}
