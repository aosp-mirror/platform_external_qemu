class RouteViewModel {
    constructor(routeModel, geocoder, autocompleter) {
        this.model = routeModel;
        this.geocoder = geocoder;
        this.autocompleter = autocompleter;
        this.isEditingRoute = false;
        this.isLoadingRoute = false;
        this.allowEdits = true;
        this.inGpxKmlState = false;
        this.routeComputedSuccesfully = true;
    }

    renderDirections(mapManager, routeJson) {
        if (this.model.isValidRoute() && !this.isLoadingRoute) {
            let renderRequest
            mapManager.clearDirections();
            if (!routeJson) {
                const travelMode = this.getTransportationMode();
                const origin = this.getOrigin().latLng;
                const destination = this.getDestination().latLng;
                let waypoints = [];
                if (travelMode !== TransportationMode.Transit) {
                    waypoints = this.getIntermediateWaypoints().map((value) => { return { location: value.latLng, stopover: false } });
                }
                renderRequest = {
                    origin,
                    destination,
                    waypoints,
                    travelMode,
                    optimizeWaypoints: true
                };
            }
            else {
                const route = JSON.parse(routeJson);
                renderRequest = route.request;
            }
            mapManager.renderDirections(renderRequest);
        }
    }

    setIsPlayingRoute(isPlaying) {
        this.model.setIsPlayingRoute(isPlaying);
    }

    getIsPlayingRoute() {
        return this.model.getIsPlayingRoute();
    }

    setEditable(allowEdits) {
        this.allowEdits = allowEdits;
    }

    setEditingRoute(editing) {
        this.isEditingRoute = editing;
    }

    setIsLoadingRoute(loading) {
        this.isLoadingRoute = loading;
    }

    setInGpxKmlState(isInState) {
        this.inGpxKmlState = isInState;
    }

    isInGpxKmlState() {
        return this.inGpxKmlState;
    }

    setRouteComputed(success) {
        this.routeComputedSuccesfully = success;
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
        for (let i = 0; i < route.routes[0].legs[0].via_waypoint.length; i++) {
            const waypoint = route.routes[0].legs[0].via_waypoint[i];
            this.addNewEmptyDestination();
            this.setDestination({
                latLng: waypoint.location,
                address: `waypoint ${i + 1}`
            });
        }
        this.addNewEmptyDestination();
        this.setDestination({
            latLng: destinationLatLng,
            address: destinationAddress
        });
    }

    saveRoute() {
        channel.objects.emulocationserver.saveRoute();
    }

    sendRouteToEmulator(route) {
        if (this.isLoadingRoute || !this.allowEdits || this.getIsPlayingRoute()) {
            return;
        }
        if (!route) {
            console.log('EMPTY ROUTE - sending route to emulator', this);
            channel.objects.emulocationserver.sendFullRouteToEmu(0, 0, null, null);
            return;
        }
        const { json, pointCount, totalDuration } = this.model.convertRouteToJson(route);
        console.log('COMPLETE ROUTE - sending route to emulator', this);
        channel.objects.emulocationserver.sendFullRouteToEmu(pointCount, totalDuration, json, this.getTransportationMode());
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
        // 6-digit precision ≈ 0.1 meter precision
        const latitude = latLng.lat().toFixed(6);
        const longitude = latLng.lng().toFixed(6);
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

    getIntermediateWaypoints() {
        let waypoints = [...this.model.waypoints];
        waypoints.shift();
        waypoints.pop();
        return waypoints;
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
        return this.model.isValidRoute() && this.totalWaypointsCount() < 4 && this.allowEdits && this.routeComputedSuccesfully;
    }

    shouldShowRemoveWaypointButtons() {
        return this.model.waypoints.length > 2;
    }

    shouldShowSwapOriginDestinationButton() {
        return this.model.waypoints.length === 2;
    }

    shouldShowSaveRouteButton() {
        return this.model.isValidRoute() && this.allowEdits && this.routeComputedSuccesfully;
    }
}

