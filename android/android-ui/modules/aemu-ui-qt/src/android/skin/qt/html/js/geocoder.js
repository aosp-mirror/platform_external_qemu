class Geocoder {
    constructor() {
        this.geocoder = new google.maps.Geocoder();
    }

    async geocodeLocation(latLng, bounds) {
        return await this._geocode({ location: latLng, bounds });
    }

    async geocodePlace(placeId, bounds) {
        return await this._geocode({ placeId, bounds });
    }

    _geocode(request) {
        const geocoder = this.geocoder;
        return new Promise((resolve, reject) => {
            console.debug("geocode called");
            geocoder.geocode(request, (results, status) => {
                incGeocodeCount();
                if (status === google.maps.GeocoderStatus.OK) {
                    resolve({
                        address: results[0].formatted_address,
                        latLng: results[0].geometry.location 
                    });
                }
                else {
                    reject(new Error(`Geocoder::geocodeLocation failed. Status=${status}`));
                }
            })
        });
    }
}