// Googleplex!
var lastLatLng = { lat: 37.4220919, lng: -122.0826088 };
var gMap;
var gSearchBox;
var gFirstPoint;
// TODO: replace the gstart and gend with WaypointInfo classes
var gStartLatLng;
var gEndLatLng;
var gStartWaypoint;
var gEndWaypoint;
var gFocusedWaypoint;
// TODO: add intermediate waypoints
var gHaveActiveRoute;
var gSearchResultMarkers = [];
var gDirectionsService;
var gDirectionsDisplay;
var gEndMarker;
var gTravelModeString = "DRIVING";
var gGeocoder;
var gAutocompleteService;
var gLastFocusedAddrBox;
var gMapClickedListener;
var gSearchContainer;
var kMaxSearchResults = 5;
var gGpxKmlPath;
var gGpxKmlStartMarker;
var gGpxKmlEndMarker;
var gIsGpxKmlPlaying;

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

function startRouteCreatorFromPoint(lat, lng, address) {
    resetRouteCreatorOverlay();
    hideAllOverlays();

    var latLng = new google.maps.LatLng(lat, lng);
    gEndWaypoint.setLatLng(latLng);
    gEndWaypoint.setMarker(new google.maps.Marker({
        map: gMap,
        position: latLng
    }));
    // Ensure that the starting point is visible
    if (!gMap.getBounds().contains(latLng)) {
        gMap.setCenter(latLng);
    }

    if (address === "") {
        gGeocoder.geocode({ 'location': latLng }, function (results, status) {
            var address = "";
            var elevation = 0.0;
            if (status === 'OK' && results[0]) {
                address = results[0].formatted_address;
                elevation = results[0].elevation;
            } else {
                address = lat + ", " + lng;
            }
            gEndWaypoint.setAddress(address);
            document.getElementById('endAddress').value = gEndWaypoint.address;
        });
    } else {
        gEndWaypoint.setAddress(address);
        document.getElementById('endAddress').value = gEndWaypoint.address;
    }

    document.getElementById('startAddress').value = "";
    showRouteCreatorOverlay(false);
}

function hideAllOverlays() {
    document.getElementById("startInfoOverlay").className = "pointOverlaySlideOut";
    document.getElementById("gpxKmlInfoOverlay").className = "pointOverlaySlideOut";
    document.getElementById("routeCreatorOverlay").className = "routeCreatorOverlayHidden";
}

function showStartInfoOverlay(address, latLng, elevation) {
    hideAllOverlays();
    setSearchBoxVisible(true);

    document.getElementById("startInfoTitle").innerHTML = address;
    var lat = latLng.lat().toFixed(4);
    var lng = latLng.lng().toFixed(4);
    subtitle = lat + ", " + lng + (elevation != null ? ", " + elevation.toFixed(4) : "");
    document.getElementById("startInfoSubtitle").innerHTML = subtitle;
    document.getElementById("startInfoOverlay").className = "pointOverlayNoBottomPanel"
}

function showGpxKmlOverlay(title, subtitle) {
    gIsGpxKmlPlaying = true;
    hideAllOverlays();
    setSearchBoxVisible(false);

    if (title != null) {
        document.getElementById("gpxKmlInfoTitle").innerHTML = title;
    }
    if (subtitle != null) {
        document.getElementById("gpxKmlInfoSubtitle").innerHTML = subtitle;
    }
    document.getElementById("gpxKmlInfoOverlay").className = "pointOverlayNoBottomPanel"
}

function hideGpxKmlOverlay() {
    hideAllOverlays();
    clearDirections();
    setSearchBoxVisible(true);
}

function showRouteCreatorOverlay(isSavedRoute) {
    gIsGpxKmlPlaying = false;
    document.getElementById("routeCreatorOverlay").className = "routeCreatorOverlay";
    if (isSavedRoute) {
        setTransportMode(gTravelModeString);
    }
}

function inRouteCreatorMode() {
    let className = document.getElementById('routeCreatorOverlay').className;
    return (className === "routeCreatorOverlay");
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
    document.getElementById('routeCreatorOverlay').className = "routeCreatorOverlayHidden"
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

    var icon = document.createElement('i');
    icon.classList.add("search-item-icon");
    icon.classList.add("material-icons");
    icon.innerText = "place";
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
    searchItem.addEventListener('click', function () {
        var placeid = this.getAttribute("data-place-id");
        var address = this.getAttribute("data-address");
        if (placeid != null) {
            gGeocoder.geocode({ 'placeId': placeid }, function (results, status) {
                if (status !== 'OK' || !results[0]) {
                    return;
                }
                document.getElementById('searchResultsOverlay').style.display = 'none';
                clearSearchResults();
                document.getElementById('routeCreatorOverlay').className = "routeCreatorOverlay";
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
    if (typeof (gLastFocusedAddrBox) === "undefined") {
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
    gGeocoder.geocode({ 'location': latLng }, function (results, status) {
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
    if (gGpxKmlPath != null) {
        gGpxKmlPath.setMap(null);
        gGpxKmlPath = null;
    }
    if (gGpxKmlStartMarker != null) {
        gGpxKmlStartMarker.setMap(null);
        gGpxKmlStartMarker = null;
    }
    if (gGpxKmlEndMarker != null) {
        gGpxKmlEndMarker.setMap(null);
        gGpxKmlEndMarker = null;
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

function setSearchBoxVisible(visible) {
    if (visible && gSearchContainer != null) {
        gMap.controls[google.maps.ControlPosition.TOP_CENTER].push(gSearchContainer);
        gSearchContainer = null;
        gMapClickedListener = google.maps.event.addListener(gMap, 'click', mapClickedEvent);
    } else if (!visible && gSearchContainer == null) {
        gSearchContainer = gMap.controls[google.maps.ControlPosition.TOP_CENTER].pop();
        google.maps.event.removeListener(gMapClickedListener);
        gMapClickedListener = null;
    }
}

function showRoutePlaybackOverlay(visible) {
    if (visible) {
        // in route playback mode
        hideAllOverlays();
        gMap.setOptions({ zoomControl: false });
        document.getElementById('locationInfoOverlay').style.display = 'flex';
        setSearchBoxVisible(false);
    } else {
        if (gIsGpxKmlPlaying) {
            document.getElementById('locationInfoOverlay').style.display = 'none';
            showGpxKmlOverlay();
        } else {
            showRouteCreatorOverlay(true);
            gMap.setOptions({ zoomControl: true });
            document.getElementById('locationInfoOverlay').style.display = 'none';
            setSearchBoxVisible(true);
        }
    }
}

function mapClickedEvent(event) {
    if (inRouteCreatorMode()) {
        if (setWaypointForEmptyAddressBox(event.latLng)) {
            calcRoute();
        }
    } else {
        showDestinationPoint(event.latLng);
    }
}

function showDestinationPoint(latLng) {
    gSearchBox.showSpinner();
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

    gGeocoder.geocode({ 'location': latLng }, function (results, status) {
        var address = "";
        var elevation = 0.0;
        if (status === 'OK' && results[0]) {
            address = results[0].formatted_address;
            elevation = results[0].elevation;
        }
        gEndWaypoint.setAddress(address);
        showStartInfoOverlay(address, latLng, elevation);
        gSearchBox.update(address);
    });
}

function zoomToGpxKmlRoute(map, path) {
    var bounds = new google.maps.LatLngBounds();
    for (var i = 0; i < path.length; ++i) {
        bounds.extend(path[i]);
    }
    map.fitBounds(bounds);
}

function showGpxKmlRouteOnMap(routeJson, title, subtitle) {
    if (routeJson.length < 2) {
        return;
    }

    clearDirections();
    showGpxKmlOverlay(title, subtitle);

    var theRoute = JSON.parse(routeJson);
    var path = theRoute.path;

    gGpxKmlStartMarker = new google.maps.Marker({
        position: path[0],
        map: gMap,
        label: {text: "A", color: "white"}
    });
    gGpxKmlEndMarker = new google.maps.Marker({
        position: path[path.length - 1],
        map: gMap,
        label: {text: "B", color: "white"}
    });
    gGpxKmlPath = new google.maps.Polyline({
        path: path,
        geodesic: true,
        strokeColor: "#709ddf",
        strokeOpacity: 1.0,
        strokeWeight: 2
    });
    gGpxKmlPath.setMap(gMap);
    zoomToGpxKmlRoute(gMap, path);
    channel.objects.emulocationserver.onSavedRouteDrawn();
}

// Callback function for Maps API
function initMap() {
    // Create a map object and specify the DOM element for display.
    gMap = new google.maps.Map(document.getElementById('map'), {
        center: lastLatLng,
        zoom: 10,
        zoomControl: true,
        controlSize: 20,
        disableDefaultUI: true
    });

    gGeocoder = new google.maps.Geocoder;
    gFirstPoint = true;
    clearDirections();
    hideAllOverlays();

    gStartWaypoint = new WaypointInfo;
    gStartWaypoint.setInputDiv(document.getElementById('startAddress'));
    gEndWaypoint = new WaypointInfo;
    gEndWaypoint.setInputDiv(document.getElementById('endAddress'));

    gBlueMarker = new google.maps.Marker({ map: null, position: lastLatLng });

    // Register a listener that sets a new marker wherever user clicks
    // on the map.
    gMapClickedListener = google.maps.event.addListener(gMap, 'click', mapClickedEvent);

    setDeviceLocation = function (lat, lng) {
        // Called from Qt code to show the blue marker to display the emulator location on the map.

        var latLng = new google.maps.LatLng(lat, lng);
        gBlueMarker.setMap(null); // Remove the old marker
        gBlueMarker = new google.maps.Marker({ map: gMap, position: latLng });

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
        document.getElementById('locationInfoOverlay').innerHTML = lat.toString() + ", " + lng.toString();
    }

    // Receive the selected route from the host
    setRouteOnMap = function (routeJson) {
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
        gSearchBox.update('');
        channel.objects.emulocationserver.onSavedRouteDrawn();
    }

    // Add Google search box
    // Create the search box and link it to the UI element.
    gSearchBox = new LocationSearchBox();
    gSearchBox.init(gMap,
        gSearchResultMarkers,
        (lastLatLng) => { showDestinationPoint(lastLatLng); },
        () => { hideAllOverlays(); }
    );

    // For the route creator
    gAutocompleteService = new google.maps.places.AutocompleteService();

    document.getElementById('closeRouteCreatorButton').addEventListener('click', function () {
        if (gDirectionsDisplay != null) {
            gDirectionsDisplay.setMap(null);
            gDirectionsDisplay = null;
        }
        document.getElementById("routeCreatorOverlay").className = "routeCreatorOverlayHidden";
        document.getElementById("startInfoOverlay").className = "pointOverlayNoBottomPanel";
        resetRouteCreatorOverlay();
        setSearchBoxVisible(true);
    });

    document.getElementById('startNavButton').style.position = 'absolute';
    document.getElementById('startNavButton').addEventListener('click', function () {
        hideAllOverlays();
        document.getElementById('endAddress').value = gEndWaypoint.address;
        document.getElementById('startAddress').value = "";
        showRouteCreatorOverlay(false);
    });

    var displaySuggestions = function (predictions, status) {
        if (status != google.maps.places.PlacesServiceStatus.OK) {
            return;
        }

        clearSearchResults();
        var numResults = predictions.length > 5 ? kMaxSearchResults : predictions.length;
        for (var i = 0; i < numResults; ++i) {
            addSearchResultItem(predictions[i].structured_formatting.main_text,
                predictions[i].structured_formatting.secondary_text,
                predictions[i].place_id);
        }
    };

    document.getElementById('startAddress').addEventListener('input', function () {
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        clearDirections();
        gStartWaypoint.invalidate();
        if (this.value === "") {
            document.getElementById('searchResultsOverlay').style.display = 'none';
            overlay.className = "routeCreatorOverlay"
        } else {
            overlay.className = "routeCreatorOverlayAutoCompleteVisible";
            document.getElementById('searchResultsOverlay').style.display = 'block';
            gLastFocusedAddrBox = this;
            gAutocompleteService.getPlacePredictions({ input: this.value }, displaySuggestions);
        }
    });

    document.getElementById('startAddress').addEventListener('focusin', function () {
        this.select();
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        document.getElementById('searchResultsOverlay').style.display = 'none';
        overlay.className = "routeCreatorOverlay"
    });

    document.getElementById('endAddress').addEventListener('focusin', function () {
        this.select();
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        document.getElementById('searchResultsOverlay').style.display = 'none';
        overlay.className = "routeCreatorOverlay"
    });

    document.getElementById('endAddress').addEventListener('input', function () {
        var overlay = document.getElementById('routeCreatorOverlay');
        clearSearchResults();
        clearDirections();
        gEndWaypoint.invalidate();
        if (this.value === "") {
            document.getElementById('searchResultsOverlay').style.display = 'none';
            overlay.className = "routeCreatorOverlay"
        } else {
            gLastFocusedAddrBox = this;
            overlay.className = "routeCreatorOverlayAutoCompleteVisible"
            document.getElementById('searchResultsOverlay').style.display = 'block';
            gAutocompleteService.getQueryPredictions({ input: this.value }, displaySuggestions);
        }
    });

    var modes = document.getElementsByName('transportMode');
    for (var i = 0; i < modes.length; ++i) {
        modes[i].addEventListener('change', function () {
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
    gDirectionsService.route(request, function (result, status) {
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
            legIdx++) {
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
