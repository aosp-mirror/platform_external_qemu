class MapController extends GoogleMapPageComponent {
    constructor(routeViewModel, routePanel, searchBox, locationPanel) {
        super();
        this.viewModel = routeViewModel;
        this.routePanel = routePanel;
        this.searchBox = searchBox;
        this.locationPanel = locationPanel;
        this.mapManager = null;
        this.currentLocationMarker = null;
        this.onRouteComputed.bind(this);
        this.onWaypointSelected.bind(this);
        this.onSearchBoxCleared.bind(this);
        this.onBeginBuildingRouteButtonClicked.bind(this);
        this.onRoutePanelClosed.bind(this);
        this.onTransportModeChanged.bind(this);
        this.onWaypointsChanged.bind(this);
        this.onLocationPanelOpened.bind(this);
        this.onLocationPanelClosed.bind(this);
    }

    onMapInitialized(mapManager, eventBus) {
        this.mapManager = mapManager;
        this.eventBus = eventBus;
        eventBus.onMapClicked((event) => this.onWaypointSelected(event));
        eventBus.onSearchBoxPlaceChanged((place) => this.onWaypointSelected({ latLng: place.geometry.location }));
        eventBus.onSearchBoxCleared(() => this.onSearchBoxCleared());
        eventBus.on('floating_action_button_clicked', () => this.onBeginBuildingRouteButtonClicked());
        eventBus.on('route_panel_closed', () => this.onRoutePanelClosed());
        eventBus.on('transportation_mode_changed', (e) => this.onTransportModeChanged(e.newMode));
        eventBus.on('waypoints_changed', () => this.onWaypointsChanged());
        eventBus.on('location_panel_opened', () => this.onLocationPanelOpened());
        eventBus.on('location_panel_closed', () => this.onLocationPanelClosed());
        eventBus.on('route_computed', (route) => this.onRouteComputed(route));
        eventBus.on('route_compute_failed', (route) => this.onRouteComputeFailed(route));
    }

    setCurrentLocation(latLng) {
        console.log('MapController::setCurrentLocation called', latLng.lat(), latLng.lng());
        if (this.currentLocationMarker) {
            this.currentLocationMarker.setMap(null);
        }
        this.currentLocationMarker = new google.maps.Marker({ map: this.mapManager.map, position: latLng });
        let blueDot = 'data:image/svg+xml, \
                <svg xmlns="http://www.w3.org/2000/svg" width="24px" height="24px" \
                        viewBox="0 0 24 24" fill="%23000000"> \
                    <circle cx="12" cy="12" r="10" fill="black" opacity=".1" /> \
                    <circle cx="12" cy="12" r="8" stroke="white" stroke-width="2" \
                        fill="%234286f5" /> \
                </svg>';
        let image = {
            url: blueDot,
            size: new google.maps.Size(24, 24),
            origin: new google.maps.Point(0, 0),
            anchor: new google.maps.Point(12, 12),
            scaledSize: new google.maps.Size(25, 25)
        };
        this.currentLocationMarker.setIcon(image);
        this.mapManager.centerMapOnLocation(latLng);
        $('#locationInfoOverlay').html(this.viewModel.formatLatLngString(latLng));
    }

    setRoute(routeJson, isSavedRoute) {
        console.log('MapController::setRoute called', routeJson);
        this.viewModel.setIsLoadingRoute(true);
        this.hideGpxKmlPanel();
        this.routePanel.close();
        this.mapManager.clearDirections();
        this.mapManager.clearMarkers();
        var loadedSuccessfully = true;
        try {
            if (routeJson.length > 0) {
                this.viewModel.setRoute(routeJson);
                this.viewModel.setEditable(!isSavedRoute);
                this.showRouteEditor();
            }
            else {
                this.viewModel.clearWaypoints();
                this.viewModel.setEditable(true);
            }
        }
        catch (error) {
            this.locationPanel.showError('Route Failed To Load', 'Oops! There was a problem loading your route. Please try again with a different route.');
            this.viewModel.setIsLoadingRoute(false);
            this.viewModel.sendRouteToEmulator();
            loadedSuccessfully = false;
        }
        finally {
            this.searchBox.update('');
            channel.objects.emulocationserver.onSavedRouteDrawn();
            this.viewModel.setIsLoadingRoute(false);
            if (loadedSuccessfully) {
                this.viewModel.renderDirections(this.mapManager, routeJson);
            }
        }
    }

    showRoutePlaybackPanel(visible) {
        console.log('MapController::showRoutePlaybackPanel called', visible);
        if (visible) {
            // in route playback mode
            this.viewModel.setIsPlayingRoute(true);
            this.locationPanel.hide();
            this.routePanel.close();
            this.searchBox.hide();
            this.mapManager.map.setOptions({ zoomControl: false });
            $('#locationInfoOverlay').css('display', 'flex');
        } else {
            this.viewModel.setIsPlayingRoute(false);
            this.searchBox.show();
            this.mapManager.map.setOptions({ zoomControl: true });
            $('#locationInfoOverlay').css('display', 'none');
        }
    }

    showGpxKmlPanel(title, subtitle) {
        this.routePanel.close();
        this.searchBox.hide();
        this.locationPanel.showTitleSubtitle(title || '', subtitle || '');
        this.viewModel.setInGpxKmlState(true);
    }

    hideGpxKmlPanel() {
        this.routePanel.close();
        this.locationPanel.hide();
        this.searchBox.show();
    }

    showRoute(routeJson, title, subtitle) {
        console.log('MapController::showRoute called', routeJson, title, subtitle);
        if (routeJson.length < 2) {
            return;
        }

        this.viewModel.setIsLoadingRoute(true);
        this.mapManager.clearDirections();
        this.showGpxKmlPanel(title, subtitle);

        const theRoute = JSON.parse(routeJson);
        const path = theRoute.path;

        this.mapManager.addMarker(path[0], {
            label: { text: "A", color: "white" }
        });

        this.mapManager.addMarker(path[path.length - 1], {
            label: { text: "B", color: "white" }
        });

        this.mapManager.addPolylinePath(path, {
            geodesic: true,
            strokeColor: "#709ddf",
            strokeOpacity: 1.0,
            strokeWeight: 2
        });

        this.mapManager.zoomToPath(path);
        channel.objects.emulocationserver.onSavedRouteDrawn();
        this.viewModel.setIsLoadingRoute(false);
    }

    displayRouteEditor(latLng, address) {
        console.log('MapController::displayRouteEditor called', latLng, address);
        this.viewModel.setIsLoadingRoute(true);
        this.locationPanel.hide();
        this.routePanel.close();
        this.viewModel.clearWaypoints();
        this.viewModel.setDestination({
            latLng,
            address
        });
        this.mapManager.addMarker(latLng);
        this.mapManager.centerMapOnLocation(latLng);
        this.routePanel.open();
        this.viewModel.setIsLoadingRoute(false);
    }

    showRouteEditor() {
        this.locationPanel.hide();
        this.routePanel.open();
    }

    moveZoomControls(position) {
        this.mapManager.map.setOptions({
            zoomControlOptions: {
                position
            }
        });
    }

    async onWaypointSelected(event) {
        if (this.viewModel.getIsPlayingRoute()) {
            return;
        }
        if (this.viewModel.isInGpxKmlState()) {
            this.searchBox.show();
            this.viewModel.setInGpxKmlState(false);
        }
        const latLng = event.latLng;
        const self = this;
        if (this.viewModel.isEditingRoute) {
            if (!this.viewModel.isValidRoute()) {
                const waypoint = await this.viewModel.geocodeLocation(latLng, this.mapManager.getBounds());
                this.viewModel.addNewWaypoint(waypoint);
            }
        }
        else {
            this.mapManager.clearMarkers();
            this.mapManager.addMarker(event.latLng);
            this.searchBox.showSpinner();
            setTimeout(async () => {
                self.searchBox.update(self.viewModel.formatLatLngString(latLng));
                const destination = await self.viewModel.geocodeLocation(latLng, self.mapManager.getBounds());
                self.searchBox.hideSpinner();
                self.searchBox.update(destination.address);
                self.locationPanel.show(destination.address, destination.latLng);
                self.viewModel.setDestination(destination);
                self.mapManager.centerMapOnLocation(latLng);
            }, 1);
        }
    }

    onWaypointsChanged() {
        this.viewModel.renderDirections(this.mapManager);
    }

    onRouteComputed(route) {
        this.viewModel.setRouteComputed(true);
        this.routePanel.routeComputed();
        this.viewModel.sendRouteToEmulator(route);
    }

    onRouteComputeFailed() {
        this.viewModel.setRouteComputed(false);
        this.routePanel.routeComputed();
        this.viewModel.sendRouteToEmulator();
        this.locationPanel.showError('Route Not Found', 'Oops! Sorry, but no route was found from your origin to your destination. Please edit the route.');
    }

    onBeginBuildingRouteButtonClicked() {
        this.showRouteEditor();
    }

    onSearchBoxCleared() {
        this.mapManager.clearMarkers();
        this.viewModel.setDestination();
        this.searchBox.update('');
        this.locationPanel.hide();
    }

    onRoutePanelClosed() {
        this.viewModel.setEditable(true);
        this.searchBox.clear();
        if (!this.viewModel.getIsPlayingRoute()) {
            this.mapManager.clearMarkers();
            this.mapManager.clearDirections();
            this.viewModel.clearWaypoints();
        }
        this.viewModel.sendRouteToEmulator();
    }

    onTransportModeChanged() {
        this.viewModel.renderDirections(this.mapManager);
    }

    onLocationPanelOpened() {
        this.moveZoomControls(google.maps.ControlPosition.RIGHT_CENTER);
    }

    onLocationPanelClosed() {
        this.moveZoomControls(google.maps.ControlPosition.RIGHT_BOTTOM);
        if (!this.viewModel.getIsPlayingRoute() && this.viewModel.isInGpxKmlState()) {
            // map is showing a gpx/kml route, then clicked on the 'x' button in the map.
            this.viewModel.sendRouteToEmulator();
            this.mapManager.clearDirections();
            this.mapManager.clearMarkers();
            this.searchBox.show();
            this.viewModel.setInGpxKmlState(false);
        }
    }
}
