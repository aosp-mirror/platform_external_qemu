class SearchBoxController extends GoogleMapPageComponent {
    constructor() {
        super();
    }

    onMapInitialized(mapManager, eventBus) {
        this.eventBus = eventBus;
        this.searchContainer = null;
        this.mapManager = mapManager;
        const self = this;
        const searchContainer = document.getElementById('search-container');
        const input = /** @type {HTMLInputElement} */ (
            document.getElementById('search-input'));
        mapManager.map.controls[google.maps.ControlPosition.TOP_CENTER].push(searchContainer);

        const autocomplete = new google.maps.places.Autocomplete(
            /** @type {HTMLInputElement} */
            (input));
        autocomplete.bindTo('bounds', mapManager.map);
        // Set the data fields to return when the user selects a place.
        autocomplete.setFields(
            ['address_components', 'geometry', 'icon', 'name']);
        // Listen for the event fired when the user selects an item from the
        // pick list.
        autocomplete.addListener('place_changed', () => self.onPlaceChanged());
        input.addEventListener('input', () => self.onSearchTextChanged(input));
        $('#search-icon').on('click', () => self.onSearchIconClicked(input));

        this.autocomplete = autocomplete;
    }

    update(address) {
        address = address || ''
        $('#search-input').val(address);
        $('#search-icon').css("display", "block");
        $('#search-spinner').css("display", "none");
        if (address.length > 0) {
            this._showCloseIcon();
        } else {
            this._showSearchIcon();
        }
    }

    showSpinner() {
        $('#search-icon').css('display', 'none');
        $('#search-spinner').css('display', 'flex');
    }

    hideSpinner() {
        $('#search-icon').css('display', 'block');
        $('#search-spinner').css('display', 'none');
    }

    clear() {
        $('#search-input').val('');
        $('#search-icon').css("display", "block");
        $('#search-spinner').css("display", "none");
        this._showSearchIcon();
    }

    show() {
        if (this.searchContainer != null) {
            this.mapManager.map.controls[google.maps.ControlPosition.TOP_CENTER]
                .push(this.searchContainer);
            this.searchContainer = null;
        }
    }

    hide() {
        if (this.searchContainer == null) {
            this.searchContainer =
                this.mapManager.map.controls[google.maps.ControlPosition.TOP_CENTER].pop();
        }
    }

    onPlaceChanged() {
        let place = this.autocomplete.getPlace();

        if (!place.geometry) {
            // User entered the name of a Place that was not suggested and
            // pressed the Enter key, or the Place Details request failed.
            // Just ignore it.
            return;
        }

        this.showSpinner();

        this.eventBus.dispatchSearchBoxPlaceChanged(place);
    }

    onSearchTextChanged(input) {
        if (input.value.length > 0) {
            this._showCloseIcon();
        } else {
            this._showSearchIcon();
        }
    }

    onSearchIconClicked(input) {
        if (input.value.length > 0) {
            input.value = "";
            this._showSearchIcon();
            this.eventBus.dispatchSearchBoxCleared();
        }
    }

    _showSearchIcon() {
        $('#search-icon').text("search");
        $('#search-icon').css("color", "#b1b1b1");
    }

    _showCloseIcon() {
        $('#search-icon').text("close");
        $('#search-icon').css("color", "#555555");
    }
}
