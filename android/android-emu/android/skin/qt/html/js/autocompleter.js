class AutoCompleter {
    constructor() {
        this.autocompleteService = new google.maps.places.AutocompleteService();
    }

    async getPlaces(searchText, bounds) {
        return await this._getPlacePredictions(searchText, bounds);
    }

    _getPlacePredictions(searchText, bounds) {
        const autocompleteService = this.autocompleteService;
        return new Promise((resolve, reject) => {
            autocompleteService.getPlacePredictions({ input: searchText, bounds }, (predictions, status) => {
                if (status === google.maps.places.PlacesServiceStatus.OK) {
                    const places = predictions.map((placeMatch) => {
                        return new PlaceModel( 
                            placeMatch.place_id, 
                            placeMatch.structured_formatting.main_text,
                            placeMatch.structured_formatting.secondary_text
                        );
                    });
                    resolve(places);
                }
                else {
                    reject(new Error(`AutocompleteService::getPlacePredictions failed for search ${searchText} with status ${status}.`));
                }
            });
        });
    }
}

class PlaceModel {
    constructor(placeId, name, address) {
        this.placeId = placeId;
        this.name = name;
        this.address = address;
    }
}