class RouteViewModel {
    constructor(routeModel, geocoder, autocompleter) {
        this.model = routeModel;
        this.geocoder = geocoder;
        this.autocompleter = autocompleter;
        this.isEditingRoute = false;
        this.allowEdits = true;
    }

    renderDirections(mapManager) {
        if (this.model.isValidRoute()) {
            mapManager.clearDirections();
            mapManager.renderDirections(this.getTransportationMode(),
                this.getOrigin(),
                this.getDestination(),
                this.model.waypoints);
        }
    }

    setEditable(allowEdits) {
        this.allowEdits = allowEdits;
    }

    setEditingRoute(editing) {
        this.isEditingRoute = editing;
    }

    setRoute(routeJson) {
        const route = JSON.parse(routeJson);
        const originLatLng = route['request']['origin']['location'];
        const destinationLatLng = route['request']['destination']['location'];
        const originAddress = route.routes[0].legs[0].start_address;
        const destinationAddress = route.routes[0].legs[0].end_address;
        const travelMode = route['request']['travelMode'];
        console.log('RouteViewModel::setRoute called', originLatLng,
            originAddress,
            destinationLatLng,
            destinationAddress,
            travelMode);
        this.model.clear();
        this.model.setTransportationMode(travelMode);        
        this.model.setOrigin({
            latLng: originLatLng,
            address: originAddress
        });
        this.model.setDestination({
            latLng: destinationLatLng,
            address: destinationAddress
        });
    }

    saveRoute() {
        channel.objects.emulocationserver.saveRoute();
    }

    addNewEmptyDestination() {
        const newDestination = new WaypointModel();
        this.model.addWaypoint(newDestination);
        return this.model.destination;
    }

    addNewWaypoint(waypoint) {
        const waypointAdded = this.model.addWaypoint(waypoint);
        return waypointAdded;
    }

    addNewDestinationFromPlaceMatch(placeMatch) {
        this.model.addWaypoint(new WaypointModel());
    }

    getTransportationMode() {
        return this.model.getTransportationMode();
    }

    setTransportationMode(newMode) {
        this.model.setTransportationMode(newMode);
    }

    getDestination() {
        return this.model.destination;
    }

    setDestination(newDestination) {
        this.model.updateWaypoint(this.model.destination, newDestination);
    }

    getDestinationAddress() {
        return this.model.destination.isEmpty() ? '' : this.model.destination.address;
    }

    getOrigin() {
        return this.model.origin;
    }

    getOriginAddress() {
        return this.model.origin.isEmpty() ? '' : this.model.origin.address;
    }

    getWaypointAddresses() {
        return this.model.waypoints.map((waypointModel) => waypointModel.address);
    }

    formatLatLngString(latLng, elevation) {
        const latitude = latLng.lat().toFixed(4);
        const longitude = latLng.lng().toFixed(4);
        return `${latitude}, ${longitude} ` + (elevation || '');
    }

    async getAutocompleteResults(searchText, bounds) {
        return await this.autocompleter.getPlaces(searchText, bounds);
    }

    async geocodePlace(placeModel, bounds) {
        return await this.geocoder.geocodePlace(placeModel.placeId, bounds);
    }

    async geocodeLocation(latLng, bounds) {
        return await this.geocoder.geocodeLocation(latLng, bounds);
    }

    swapOriginWithDestination() {
        this.model.swapOriginWithDestination();
    }

    updateWaypointWithGecodedPlace(place, waypoint) {
        this.model.updateWaypoint(waypoint, { latLng: place.latLng, address: place.address });
    }

    removeOrigin() {
        this.model.removeWaypoint(0);
    }

    removeDestination() {
        this.model.removeWaypoint(this.model.waypoints.length - 1);
    }

    removeWaypoint(id) {
        this.model.removeWaypoint(this.getWaypointIndexFromId(id));
    }

    clearWaypoint(id) {
        this.model.clearWaypoint(this.getWaypointIndexFromId(id));
    }

    clearWaypoints() {
        this.model.clear();
    }

    totalWaypointsCount() {
        return this.model.waypoints.length;
    }

    intermediateWaypointsCount() {
        const count = this.model.waypoints.length - 2;
        if (count < 0) {
            throw new Error(`intermediateWaypointsCount cannot be calculated. Invalid number of waypoints in route - ${count} [${this.model}]`);
        }
        return count;
    }

    getLastIntermediateWaypoint() {
        const waypointIndex = this.getLastIntermediateWaypointIndex();
        return waypointIndex >= 0 ? this.model.waypoints[waypointIndex] : null;
    }

    getLastIntermediateWaypointIndex() {
        return this.model.waypoints.length > 2 ? this.model.waypoints.length - 2 : -1;
    }

    getWaypointIndexFromId(id) {
        if (id === 'origin-address') {
            return 0;
        }
        else if (id === 'destination-address') {
            return this.totalWaypointsCount() - 1;
        }
        else {
            return parseInt(id[3]);
        }
    }

    isValidRoute() {
        return this.model.isValidRoute();
    }

    shouldShowSearchResultsContainer(text) {
        return text.length !== 0;
    }

    shouldShowAddDestinationButton() {
        return this.model.isValidRoute() && this.totalWaypointsCount() < 4 && this.allowEdits;
    }

    shouldShowRemoveWaypointButtons() {
        return this.model.waypoints.length > 2;
    }

    shouldShowSwapOriginDestinationButton() {
        return this.model.waypoints.length === 2;
    }

    shouldShowSaveRouteButton() {
        return this.model.isValidRoute() && this.allowEdits;
    }
}

