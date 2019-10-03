class RoutePanelController extends GoogleMapPageComponent {
    constructor(routeViewModel) {
        super();
        this.animatePanelExpandClassName = 'route-container';
        this.animatePanelCollapseClassName = 'route-container-collapsed';
        this.selectedTransportModeCssClassName = 'round-icon-selected';
        this.normalTransportModeCssClassName = 'round-icon';
        this.viewModel = routeViewModel;
        this.autocompleteMatches = null;
    }

    onMapInitialized(mapManager, eventBus) {
        const self = this;
        this.eventBus = eventBus;
        this.mapManager = mapManager;
        this.viewModel.setTransportationMode(TransportationMode.Driving);

        $('#transport-mode-DRIVING').click(() => this.viewModel.setTransportationMode(TransportationMode.Driving));
        $('#transport-mode-TRANSIT').click(() => this.viewModel.setTransportationMode(TransportationMode.Transit));
        $('#transport-mode-WALKING').click(() => this.viewModel.setTransportationMode(TransportationMode.Walking));
        $('#transport-mode-BICYCLING').click(() => this.viewModel.setTransportationMode(TransportationMode.Bicycling));

        $('#route-panel :input')
            .on('input', (e) => self.onWaypointAddressInputTextChanged($(e.currentTarget)))
            .focus((e) => self.onWaypointAddressInputFocus($(e.currentTarget)));

        $('#add-destination-icon').click(() => self.onAddDestination());
        $('#add-destination-label').click(() => self.onAddDestination());
        $('#swap-origin-destination-button').click(() => self.swapOriginWithDestination());
        $('#origin-remove-icon').click(() => self.onRemoveOrigin());
        $('#prototype-destination-remove-icon').click((e) => self.onRemoveWaypoint($(e.currentTarget)));
        $('#destination-remove-icon').click(() => self.onRemoveDestination());
        $('#save-route-button').click(() => self.onSaveRouteButtonClicked());
        $('#route-panel-close').click(() => this.close());

        eventBus.on('transportation_mode_changed', (e) => {
            self.deselectTransportationMode(e.oldMode);
            self.selectTransportationMode(e.newMode);
        });
        eventBus.on('waypoints_changed', (e) => self.onWaypointsChanged(e));

        $('#origin-address').data('waypoint', this.viewModel.getOrigin());
        $('#destination-address').data('waypoint', this.viewModel.getDestination());

        this.showRemoveWaypointButtons(false);
        this.showSwapOriginDestinationButton(true);
        this.showSaveRouteButton(false);
    }

    open() {
        this.viewModel.setEditingRoute(true);
        this.clearWaypointElements();
        this._element().removeClass().addClass(this.animatePanelExpandClassName);

        $('#origin-address').val(this.viewModel.getOriginAddress());
        if (this.viewModel.getOriginAddress().length === 0) {
            $('#origin-address').focus();
        }
        $('#destination-address').val(this.viewModel.getDestinationAddress());

        this.showRemoveWaypointButtons(false);
        this.showSwapOriginDestinationButton(true);
        this.selectTransportationMode(this.viewModel.getTransportationMode());
        this.showSaveRouteButton(this.viewModel.shouldShowSaveRouteButton());
        this.showAddDestinationButton(this.viewModel.shouldShowAddDestinationButton());
        this.showSwapOriginDestinationButton(this.viewModel.shouldShowSwapOriginDestinationButton());
        this.showActionButtonsContainer(this.viewModel.allowEdits);
    }

    close() {
        this.viewModel.setEditingRoute(false);
        this._element().removeClass().addClass(this.animatePanelCollapseClassName);
        this.showAddDestinationButton(false);
        this.clearWaypointElements();
        this.eventBus.dispatch('route_panel_closed');
    }

    showSearchResultsContainer(visible) {
        if (visible) {
            $('#search-results-container').show();
            this._element().css('height', '100%');
        }
        else {
            $('#search-results-container').hide();
            this._element().css('height', 'auto');
        }
    }

    showAddDestinationButton(visible) {
        if (visible) {
            $('#add-destination-icon').show();
            $('#add-destination-label').show();
        }
        else {
            $('#add-destination-icon').hide();
            $('#add-destination-label').hide();
        }
    }

    showRemoveWaypointButtons(visible) {
        if (visible) {
            $('#origin-remove-icon').show();
            $('#destination-remove-icon').show();
        }
        else {
            $('#origin-remove-icon').hide();
            $('#destination-remove-icon').hide();
        }
    }

    showSwapOriginDestinationButton(visible) {
        if (visible) {
            $('#swap-origin-destination-button').show();
        }
        else {
            $('#swap-origin-destination-button').hide();
        }
    }

    showSaveRouteButton(visible) {
        if (visible) {
            $('#save-route-button').show();
        }
        else {
            $('#save-route-button').hide();
        }
    }

    showActionButtonsContainer(visible) {
        if (visible) {
            $('#action-buttons-container').show();
        }
        else {
            $('#action-buttons-container').hide();
        }
    }

    swapOriginWithDestination() {
        this.viewModel.swapOriginWithDestination();
        $('#origin-address').val(this.viewModel.getOriginAddress());
        $('#destination-address').val(this.viewModel.getDestinationAddress());
    }

    async displayAutocompleteMatches(searchTextInputElement, placeModels) {
        console.log('RoutePanelController::displayAutocompleteMatches() called. placeModels = ', placeModels);
        this.clearSearchResultItems();
        const matchesToDisplay = placeModels.slice(0, 5);
        for (let i = 0; i < matchesToDisplay.length; i++) {
            await this.addSearchResultItem(i, matchesToDisplay[i], searchTextInputElement);
        }
        this.autocompleteMatches = matchesToDisplay;
    }

    async addSearchResultItem(index, placeModel, searchTextInputElement) {
        const searchResultItemId = 'search-result-item-' + index;
        $('#prototype-search-result-item')
            .clone()
            .appendTo('#search-results-container')
            .removeAttr('id')
            .attr('id', searchResultItemId)
            .click(() => this.onAutoCompleteSearchItemClicked(searchTextInputElement, placeModel))
            .show();
        $(`#${searchResultItemId} .search-result-item-name`).contents().get(0).nodeValue = placeModel.name;
        $(`#${searchResultItemId} .search-result-item-address`).text(placeModel.address);
    }

    clearSearchResultItems() {
        console.log('RoutePanelController::clearSearchResultItems() called');
        $('div[id^="search-result-item-"]').remove();
    }

    clearWaypointElements() {
        $('[id^=wp-]').remove();
    }

    async onWaypointAddressInputTextChanged(addressInputElement) {
        const text = addressInputElement.val();
        const isSearchResultsContainerVisible = this.viewModel.shouldShowSearchResultsContainer(text);

        this.showSearchResultsContainer(isSearchResultsContainerVisible);

        if (isSearchResultsContainerVisible) {
            const matchingPlaceModels = await this.viewModel.getAutocompleteResults(text, this.mapManager.getBounds());
            await this.displayAutocompleteMatches(addressInputElement, matchingPlaceModels);
        }
        else {
            this.viewModel.clearWaypoint(addressInputElement.attr('id'));
        }
    }

    onWaypointAddressInputFocus(addressInputElement) {
        addressInputElement.select();
    }

    async onAutoCompleteSearchItemClicked(searchTextInputElement, placeModel) {
        searchTextInputElement.val(placeModel.name);
        this.showSearchResultsContainer(false);
        const geocodedPlace = await this.viewModel.geocodePlace(placeModel);
        searchTextInputElement.val(geocodedPlace.address);
        this.viewModel.updateWaypointWithGecodedPlace(geocodedPlace, searchTextInputElement.data('waypoint'));
    }

    onAddDestination() {
        this.showAddDestinationButton(false);
        // how many waypoints currently exist (minus the origin and destination)?
        const waypointIndex = this.viewModel.intermediateWaypointsCount() + 1;
        const newDestination = this.viewModel.addNewEmptyDestination();
        const previousDestination = this.viewModel.getLastIntermediateWaypoint();

        console.log('RoutePanelController::onAddDestination called', waypointIndex);
        // copy the prototype waypoint elements and insert them before the destination elements
        $('#prototype-start-dot-icon')
            .clone()
            .removeAttr('id')
            .attr('id', `wp-${waypointIndex}-start-dot-icon`)
            .insertBefore('#destination-icon')
            .show();

        $('#prototype-waypoints-icon')
            .clone()
            .removeAttr('id')
            .attr('id', `wp-${waypointIndex}-icon`)
            .insertBefore('#destination-icon')
            .show();

        const newAddressInputElement = $('#prototype-address-input')
            .clone(true, true)
            .removeAttr('id')
            .attr('id', `wp-${waypointIndex}-address-input`)
            .insertBefore('#destination-address')
            .data('waypoint', previousDestination)
            .show();

        $('#prototype-destination-remove-icon')
            .clone(true, true)
            .removeAttr('id')
            .attr('id', `wp-${waypointIndex}-remove-icon`)
            .insertBefore('#destination-remove-icon')
            .show();

        newAddressInputElement.val(previousDestination.address);
        $('#destination-address').val(newDestination.address);
        $('#destination-address').focus();
    }

    onWaypointsChanged() {
        const waypointAddresses = this.viewModel.getWaypointAddresses();
        $('#route-panel :input')
            .filter(':visible')
            .each((index, val) => {
                const inputElement = $('#' + val.id);
                inputElement.val(waypointAddresses[index]);
            });
        this.showAddDestinationButton(this.viewModel.shouldShowAddDestinationButton());
        this.showRemoveWaypointButtons(this.viewModel.shouldShowRemoveWaypointButtons());
        this.showSwapOriginDestinationButton(this.viewModel.shouldShowSwapOriginDestinationButton());
        this.showSaveRouteButton(this.viewModel.shouldShowSaveRouteButton());
    }

    onRemoveOrigin() {
        $('#origin-address').val(this.viewModel.getOriginAddress());
        $('[id^=wp-1-]').remove();
        this.viewModel.removeOrigin();
    }

    onRemoveDestination() {
        const index = this.viewModel.getLastIntermediateWaypointIndex();
        $('#destination-address').val(this.viewModel.getDestinationAddress());
        $(`[id^=wp-${index}-]`).remove();
        this.viewModel.removeDestination();
    }

    onRemoveWaypoint(addressInputElement) {
        const index = this.viewModel.getWaypointIndexFromId(addressInputElement.attr('id'));
        $(`[id^=wp-${index}-]`).remove();
        this.viewModel.removeWaypoint(addressInputElement.attr('id'));
    }

    onSaveRouteButtonClicked() {
        console.log('RoutePanelController - Save Route Button clicked');
        this.viewModel.saveRoute();
    }

    deselectTransportationMode(mode) {
        $('#transport-mode-' + mode).removeClass(this.selectedTransportModeCssClassName).addClass(this.normalTransportModeCssClassName)
    }

    selectTransportationMode(mode) {
        $('#transport-mode-' + mode).removeClass(this.normalTransportModeCssClassName).addClass(this.selectedTransportModeCssClassName)
    }

    routeComputed() {
        this.showSaveRouteButton(this.viewModel.shouldShowSaveRouteButton());
        this.showAddDestinationButton(this.viewModel.shouldShowAddDestinationButton());
    }

    _element() { return $('#route-panel'); }
}

