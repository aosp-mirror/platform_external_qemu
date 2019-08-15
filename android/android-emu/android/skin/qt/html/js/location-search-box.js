class LocationSearchBox {
    init(map) {
        // Add Google search box
        // Create the search box and link it to the UI element.
        var searchContainer = document.getElementById('search-container');
        var input = /** @type {HTMLInputElement} */ (
            document.getElementById('search-input'));
        map.controls[google.maps.ControlPosition.TOP_CENTER].push(searchContainer);

        var autocomplete = new google.maps.places.Autocomplete(
            /** @type {HTMLInputElement} */
            (input));
        autocomplete.bindTo('bounds', map);
        // Set the data fields to return when the user selects a place.
        autocomplete.setFields(
            ['address_components', 'geometry', 'icon', 'name']);

        // Listen for the event fired when the user selects an item from the
        // pick list.
        autocomplete.addListener('place_changed', function () {
            // Clear the red pin and any existing search markers
            if (gPendingMarker != null) {
                gPendingMarker.setMap(null);
            }
            for (marker in gSearchResultMarkers) {
                marker.setMap(null);
            }
            gSearchResultMarkers = [];

            var place = autocomplete.getPlace();
            if (!place.geometry) {
                // User entered the name of a Place that was not suggested and
                // pressed the Enter key, or the Place Details request failed.
                // Just ignore it.
                return;
            }

            lastLatLng = place.geometry.location;
            showPendingLocation(lastLatLng.lat(), lastLatLng.lng());
            sendAddress(lastLatLng);

            // If the place has a geometry, then present it on a map.
            if (place.geometry.viewport) {
                map.fitBounds(place.geometry.viewport);
            } else {
                map.setCenter(place.geometry.location);
                map.setZoom(17);  // Why 17? Because it looks good.
            }
        });

        document.getElementById('search-icon').addEventListener('click', function () {
            if (input.value.length > 0) {
                input.value = "";
                this.innerText = "search";
                gPointOverlay.hide()
                if (gPendingMarker != null) {
                    gPendingMarker.setMap(null);
                }
            }
        });
        input.addEventListener('input', function () {
            var searchIcon = $('#search-icon');
            if (input.value.length > 0) {
                searchIcon.text("close");
            } else {
                searchIcon.text("search");
            }
        });
    }

    update(address) {
        $('#search-input').val(address);
        $('#search-icon').text("close");
        $('#search-icon').css("display", "block");
        $('#search-spinner').css("display", "none");
    }

    showSpinner() {
        $('#search-icon').css('display', 'none');
        $('#search-spinner').css('display', 'flex');
    }

    clear() {
        $('#search-icon').text("search");
    }
}
