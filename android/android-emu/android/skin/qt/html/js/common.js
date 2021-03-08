let _mapManager = null;
let _eventBus = null;
let _pageComponents = null;

function initMap() {
    _eventBus = new GoogleMapEventBus();
    _mapManager = new GoogleMapManager(_eventBus);
    _mapManager.init();

    _pageComponents = buildMapPageComponents(_eventBus);

    console.log('components found', _pageComponents);

    for (let i = 0; i < _pageComponents.length; i++) {
        console.log('component=', _pageComponents[i]);
        _pageComponents[i].onMapInitialized(_mapManager, _eventBus);
    }
}
