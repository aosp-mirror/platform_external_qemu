class LocationPointOverlay {
    constructor() {
        this.slideInCssClassName = 'pointOverlaySlideIn';
        this.slideOutCssClassName = 'pointOverlaySlideOut';
        this.hideBottomPanelCssClassName = 'pointOverlayHideBottomPanel';
    }

    show(locationAddress, latLng, elevation, hideSavePoint) {
        console.log('showing point overlay', locationAddress, latLng, elevation, hideSavePoint)
        $("#pointTitle").html(locationAddress);
        var latitude = latLng.lat().toFixed(4);
        var longitude = latLng.lng().toFixed(4);
        var subtitle = latitude + ", " + longitude + (elevation != null ? ", " + elevation.toFixed(4) : "");
        $("#pointSubtitle").html(subtitle);
        if (hideSavePoint) {
            $("#pointOverlay").removeClass().addClass(this.hideBottomPanelCssClassName);
        }
        else {
            $("#pointOverlay").removeClass().addClass(this.slideInCssClassName);
        }
        $("#lowerPanel").css("display", hideSavePoint ? "none" : "flex");
        gMap.setOptions({
            zoomControlOptions: {
                position: google.maps.ControlPosition.RIGHT_CENTER
            }
        });
    }

    showMessage(message, timeout) {
        console.log('showing point overlay', message);
        $("#pointTitle").html(message);
        $("#pointSubtitle").html('');
        $("#pointOverlay").removeClass().addClass(this.hideBottomPanelCssClassName);
        $("#lowerPanel").css("display", "none");
        if (timeout > 0) {
            setTimeout(() => {
                this.hide();
            }, timeout);
        }
        gMap.setOptions({
            zoomControlOptions: {
                position: google.maps.ControlPosition.RIGHT_CENTER
            }
        });
    }

    hide() {
        console.log('hiding point overlay')
        $("#pointOverlay").removeClass().addClass(this.slideOutCssClassName);
        gMap.setOptions({
            zoomControlOptions: {
                position: google.maps.ControlPosition.RIGHT_BOTTOM
            }
        });
    }
}
