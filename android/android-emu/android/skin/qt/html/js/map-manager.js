class GoogleMapManager {
    constructor(eventBus, mapElementId, initialLocation) {
        this._mapElementId = mapElementId || 'map';
        this.map = null;
        this.geocoder = null;
        this.autocomplete = null;
        this._initialLocation = initialLocation || { lat: 37.4220919, lng: -122.0826088 }; // google plex
        this._markers = new Array();
        this.defaultZoomInLevel = 17;
        this.eventBus = eventBus;
    }

    init() {
        const self = this;
        this.map = new google.maps.Map(document.getElementById(this._mapElementId), {
            center: self._initialLocation,
            zoom: 10,
            zoomControl: true,
            controlSize: 18,
            disableDefaultUI: true
        });
        this.geocoder = new google.maps.Geocoder();
        this.autocomplete = new google.maps.places.AutocompleteService();
        this.directionsService = new google.maps.DirectionsService();

        google.maps.event.addListener(this.map, 'click', (event) => {
            self.eventBus.dispatchMapClicked(event);
        });
    }

    addMarker(latLng, properties) {
        const newMarker = new google.maps.Marker({
            map: this.map,
            position: latLng,
            ...properties
        });
        this._markers.push(newMarker);
        return newMarker;
    }

    addPolylinePath(path, properties) {
        const newPolyline = new google.maps.Polyline({
            path: path,
            map: this.map,
            ...properties
        });
        this._markers.push(newPolyline);
        return newPolyline;
    }

    clearMarkers() {
        for (let i = 0; i < this._markers.length; i++) {
            this._markers[i].setMap(null);
        }
        this._markers = new Array();
    }

    zoomToPath(path) {
        let bounds = new google.maps.LatLngBounds();
        for (let i = 0; i < path.length; ++i) {
            bounds.extend(path[i]);
        }
        this.map.fitBounds(bounds);
    }

    centerMapOnLocation(latLng) {
        if (!latLng) {
            console.log('GoogleMapManager::centerMapOnLocation failed', latLng);
        }
        this.map.panTo(latLng);
    }

    centerMapOnPlace(place) {
        if (place.geometry.viewport) {
            this.map.fitBounds(place.geometry.viewport);
        }
        else if (place.geometry.location) {
            this.map.panTo(place.geometry.location);
            this.map.setZoom(this.defaultZoomInLevel);
        }
        else {
            console.log('GoogleMapManager::centerMapOnPlace failed', place);
        }
    }

    findPlaces(searchTextInputElement, placeNameMatchesFoundHandler) {
        this.autocomplete.getPlacePredictions({ input: searchTextInputElement.val() },
            (predictions, status) => {
                if (status !== google.maps.places.PlacesServiceStatus.OK) {
                    console.log('GoogleMapManager::findPlaces - autocomplete.getPlacePredictions failed.', searchText, status);
                    return;
                }
                placeNameMatchesFoundHandler(searchTextInputElement, predictions);
            });
    }

    fitWaypoints(waypoints) {
        const locations = waypoints.map((value) => value.latLng);
        let bounds = new google.maps.LatLngBounds();
        for (let i = 0; i < locations.length; i++) {
            bounds.extend(locations[i]);
        }
        if (this.map.getProjection()) {
            const offsetx = 0;
            const offsety = -800;

            const point2 = this.map.getProjection().fromLatLngToPoint(bounds.getSouthWest());
            this.map.fitBounds(bounds);

            const point1 = new google.maps.Point(
                ((typeof (offsetx) == 'number' ? offsetx : 0) / Math.pow(2, this.map.getZoom())) || 0,
                ((typeof (offsety) == 'number' ? offsety : 0) / Math.pow(2, this.map.getZoom())) || 0
            );

            const newPoint = this.map.getProjection().fromPointToLatLng(new google.maps.Point(
                point1.x - point2.x,
                point1.y + point2.y
            ));

            bounds.extend(newPoint);
            this.map.fitBounds(bounds);
        }
    }

    renderDirections(travelMode, originWaypoint, destinationWaypoint, mapWaypoints) {
        const self = this;
        const origin = originWaypoint.latLng;
        const destination = destinationWaypoint.latLng;
        let waypoints = [];
        if (travelMode !== TransportationMode.Transit) {
            mapWaypoints.map((value) => { return { location: value.latLng, stopover: false } });
            waypoints.shift();
            waypoints.pop();    
        }
        console.log("GoogleMapManager::renderDirections called", travelMode, origin, destination, waypoints);
        const renderRequest = {
            origin,
            destination,
            waypoints,
            travelMode,
            optimizeWaypoints: true
        };
        this.clearDirections();
        this.clearMarkers();
        this.directionsRenderer = new google.maps.DirectionsRenderer();
        this.directionsRenderer.setMap(this.map);
        this.directionsService.route(renderRequest, (result, status) => {
            if (status !== 'OK') {
                return;
            }

            self.directionsRenderer.setDirections(result);

            // Calculate the number of points and the total duration of the route
            let numPoints = 0;
            let totalDuration = 0.0;
            for (let legIdx = 0;
                legIdx < result.routes[0].legs.length;
                legIdx++) {
                totalDuration += result.routes[0].legs[legIdx].duration.value;

                let numSteps = result.routes[0].legs[legIdx].steps.length;
                for (let stepIdx = 0; stepIdx < numSteps; stepIdx++) {
                    numPoints += result.routes[0].legs[legIdx].steps[stepIdx].path.length;
                }
                numPoints -= (numSteps - 1); // Don't count the duplicate first points
            }

            let fullResult = JSON.stringify(result);
            channel.objects.emulocationserver.sendFullRouteToEmu(numPoints, totalDuration, fullResult, travelMode);
            self.eventBus.dispatch("routeRenderingCompleted", { renderRequest, result });
        });
    }

    clearDirections() {
        if (this.directionsRenderer) {
            this.directionsRenderer.setMap(null);
            this.directionsRenderer = null;
        }
        this.eventBus.dispatch("routeCleared", {});
    }

    getBounds() {
        this.map.getBounds();
    }
}

class GoogleMapEventBus extends EventBus {
    constructor() {
        super();
        this.mapClickedEventName = "mapClicked";
        this.searchBoxPlaceChangedEventName = "searchBoxPlaceChanged";
        this.searchBoxClearedEventName = "searchBoxCleared";
    }

    dispatchMapClicked(clickEvent) {
        this.dispatch(this.mapClickedEventName, clickEvent);
    }

    dispatchSearchBoxPlaceChanged(place) {
        this.dispatch(this.searchBoxPlaceChangedEventName, place);
    }

    dispatchSearchBoxCleared() {
        this.dispatch(this.searchBoxClearedEventName, {});
    }

    onMapClicked(clickEventListener) {
        this.on(this.mapClickedEventName, clickEventListener);
    }

    onSearchBoxPlaceChanged(searchBoxPlaceChangedListener) {
        this.on(this.searchBoxPlaceChangedEventName, searchBoxPlaceChangedListener);
    }

    onSearchBoxCleared(searchBoxClearedListener) {
        this.on(this.searchBoxClearedEventName, searchBoxClearedListener);
    }
}

class GoogleMapPageComponent {
    onMapInitialized(mapManager, eventBus) {
        console.log('GoogleMapPageComponent::onMapInitialized called', mapManager, eventBus);
    }
}