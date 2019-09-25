function buildMapPageComponents(eventBus) {
    let pageComponents = new Array();
    const geocoder = new Geocoder();
    const autocompleter = new AutoCompleter();
    const routeModel = new RouteModel(eventBus);
    const routeViewModel = new RouteViewModel(routeModel, geocoder, autocompleter);
    const routePanelController = new RoutePanelController(routeViewModel);
    const searchBox = new SearchBoxController();
    const locationPanel = new LocationPanelController('location-no-action-button-container',
    'location-container-collapsed');
    const mapController = new MapController(routeViewModel, routePanelController, searchBox, locationPanel);

    pageComponents.push(mapController);

    pageComponents.push(routePanelController);

    pageComponents.push(searchBox);

    pageComponents.push(locationPanel);

    //
    // Qt <-> Javascript Bridge Functions
    //
    // Called from Qt code to show the blue marker to display the emulator location on the map.
    setDeviceLocation = (lat, lng) => {
        mapController.setCurrentLocation(new google.maps.LatLng(lat, lng));
    }
    // Receive the selected route from the host
    setRouteOnMap = (routeJson, isSavedRoute) => {
        mapController.setRoute(routeJson, isSavedRoute);
    }

    showRoutePlaybackOverlay = (visible) => {
        mapController.showRoutePlaybackPanel(visible);
    }

    showGpxKmlRouteOnMap = (routeJson, title, subtitle) => {
        mapController.showRoute(routeJson, title, subtitle);
    }

    startRouteCreatorFromPoint = (lat, lng, address) => {
        mapController.displayRouteEditor(new google.maps.LatLng(lat,lng), address);
    }

    return pageComponents;
}
