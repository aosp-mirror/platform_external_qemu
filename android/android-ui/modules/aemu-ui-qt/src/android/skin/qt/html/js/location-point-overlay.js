class LocationPointOverlay {
    constructor() {
        this.slideInCssClassName = 'pointOverlaySlideIn';
        this.slideOutCssClassName = 'pointOverlaySlideOut';
        this.hideBottomPanelCssClassName = 'pointOverlayHideBottomPanel';
    }

    show(locationAddress, latLng, hideSavePoint) {
        console.log(`showing point overlay addr=[${locationAddress}] ` +
                    `latLng=[${latLng.lat()}, ${latLng.lng()}], hideSavePoint=${hideSavePoint}`)
        $("#pointTitle").html(locationAddress);
        var latitude = latLng.lat().toFixed(6);
        var longitude = latLng.lng().toFixed(6);
        var subtitle = latitude + ", " + longitude;
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
