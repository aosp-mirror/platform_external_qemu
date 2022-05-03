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
            controlSize: 20,
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

    renderDirections(renderRequest) {
        console.log("GoogleMapManager::renderDirections called", renderRequest);
        const self = this;
        this.clearDirections();
        this.clearMarkers();
        this.directionsRenderer = new google.maps.DirectionsRenderer();
        this.directionsRenderer.setMap(this.map);
        this.directionsService.route(renderRequest, (result, status) => {
            if (status !== 'OK') {
                this.eventBus.dispatch('route_compute_failed', result);
                return;
            }

            self.directionsRenderer.setDirections(result);

            this.eventBus.dispatch('route_computed', result);
        });
    }

    clearDirections() {
        if (this.directionsRenderer) {
            this.directionsRenderer.setMap(null);
            this.directionsRenderer = null;
        }
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