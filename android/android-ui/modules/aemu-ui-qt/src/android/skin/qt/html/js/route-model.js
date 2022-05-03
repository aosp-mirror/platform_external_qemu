const TransportationMode = Object.freeze({
    Driving: 'DRIVING',
    Transit: 'TRANSIT',
    Walking: 'WALKING',
    Bicycling: 'BICYCLING'
});

class RouteModel {
    constructor(eventBus) {
        this.origin = new WaypointModel();
        this.destination = new WaypointModel();
        this.transportationMode = TransportationMode.Driving;
        this.waypoints = [this.origin, this.destination]; // array of WaypointModels
        this.eventBus = eventBus;
        this.isPlayingRoute = false;
    }

    getIsPlayingRoute() {
        return this.isPlayingRoute;
    }

    setIsPlayingRoute(isPlaying) {
        this.isPlayingRoute = isPlaying;
    }

    getTransportationMode() {
        return this.transportationMode;
    }

    setTransportationMode(newMode) {
        const oldMode = this.transportationMode;
        if (oldMode === newMode) { return; }

        this.transportationMode = newMode;
        this.eventBus.dispatch('transportation_mode_changed', { newMode, oldMode });
    }

    setDestination(waypointModel) {
        this.destination.copy(waypointModel);
        this.eventBus.dispatch('waypoints_changed', this);
    }

    setOrigin(waypointModel) {
        this.origin.copy(waypointModel);
        this.eventBus.dispatch('waypoints_changed', this);
    }

    addWaypoint(waypointModel) {
        let waypointAdded
        if (this.origin.isEmpty()) {
            this.origin.copy(waypointModel);
            waypointAdded = this.origin;
        }
        else if (this.destination.isEmpty()) {
            this.destination.copy(waypointModel);
            waypointAdded = this.destination;
        }
        else {
            const waypointToAdd = new WaypointModel();
            waypointToAdd.copy(this.destination);
            this.destination.copy(waypointModel);
            this.waypoints.splice(this.waypoints.length - 1, 0, waypointToAdd);
            waypointAdded = waypointToAdd;
        }
        this.eventBus.dispatch('waypoints_changed', this);
        return waypointAdded;
    }

    updateWaypoint(waypoint, properties) {
        const index = this.waypoints.indexOf(waypoint);
        if (index === -1) {
            throw new Error('ERROR: updateWaypoint failed. Waypoint does not exist in collection.', waypoint, properties);
        }
        if (properties === null) {
            throw new Error('ERROR: updateWaypoint failed. Null properties object passed.', waypoint);
        }
        waypoint.copy(properties);

        this.eventBus.dispatch('waypoints_changed', this);
    }

    removeWaypoint(index) {
        if (index < 0 || index >= this.waypoints.length) {
            throw new Error(`removeWaypoint - Error: index out of bounds ${index} ${this.waypoints}`);
        }

        if (index === 0) {
            if (this.waypoints.length > 2) {
                this.origin.copy(this.waypoints[1]);
                this.waypoints.splice(1, 1);
            }
            else {
                this.origin.clear();
            }
        }
        else if (index === this.waypoints.length - 1) {
            if (this.waypoints.length > 2) {
                const waypointIndex = this.waypoints.length - 2;
                this.destination.copy(this.waypoints[waypointIndex]);
                this.waypoints.splice(waypointIndex, 1);
            }
            else {
                this.destination.clear();
            }
        }
        else {
            this.waypoints.splice(index, 1);
        }

        this.eventBus.dispatch('waypoints_changed', this);
    }

    clearWaypoint(index) {
        if (index < 0 || index >= this.waypoints.length) {
            throw new Error(`clearWaypoint - Error: index out of bounds ${index} ${this.waypoints}`);
        }
        this.waypoints[index].clear();

        this.eventBus.dispatch('waypoints_changed', this);
    }

    swapOriginWithDestination() {
        const tempWaypoint = new WaypointModel();
        tempWaypoint.copy(this.origin);
        this.origin.copy(this.destination);
        this.destination.copy(tempWaypoint);

        this.eventBus.dispatch('waypoints_changed', this);
    }

    clear() {
        this.origin.clear();
        this.destination.clear();
        this.waypoints = [this.origin, this.destination];
        this.eventBus.dispatch('waypoints_changed', this);
    }

    convertRouteToJson(route) {
        // Calculate the number of points and the total duration of the route
        let pointCount = 0;
        let totalDuration = 0.0;
        for (let legIdx = 0;
            legIdx < route.routes[0].legs.length;
            legIdx++) {
            totalDuration += route.routes[0].legs[legIdx].duration.value;

            let numSteps = route.routes[0].legs[legIdx].steps.length;
            for (let stepIdx = 0; stepIdx < numSteps; stepIdx++) {
                pointCount += route.routes[0].legs[legIdx].steps[stepIdx].path.length;
            }
            pointCount -= (numSteps - 1); // Don't count the duplicate first points
        }
        return { json: JSON.stringify(route), pointCount, totalDuration };
    }

    isValidRoute() {
        for (let i = 0; i < this.waypoints.length; i++) {
            console.log('isEmpty', this.waypoints[i], this.waypoints[i].isEmpty());
            if (this.waypoints[i].isEmpty()) { return false; }
        }
        return true;
    }
}

class WaypointModel {
    constructor(latLng, placeId, address, elevation, marker) {
        this.latLng = latLng;
        this.placeId = placeId;
        this.address = address;
        this.elevation = elevation;
        this.marker = marker;
    }

    isEmpty() {
        return !this.latLng || (!this.placeId && !this.address);
    }

    copy(waypointModel) {
        Object.assign(this, waypointModel);
    }

    clear() {
        this.latLng = null;
        this.placeId = null;
        this.address = null;
        this.elevation = null;
        this.marker = null;
    }
}
