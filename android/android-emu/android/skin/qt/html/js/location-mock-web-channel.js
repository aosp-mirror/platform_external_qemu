class MockEmuLocationServer {
    constructor() {
        console.log("Running outside emulator. Creating fake emulocationserver.");
        this.lat = 0.0;
        this.lng = 0.0;
        this.addr = "";
    }

    map_savePoint() {
        console.log("map_savePoint() faked");
        showPendingLocation(this.lat, this.lng, this.addr);
    }

    sendLocation(lat, lng, addr) {
        console.log("sendLocation(" + lat + ", " + lng + ", " + addr + ") faked");
        this.lat = lat;
        this.lng = lng;
        this.addr = addr;
    }

    sendFullRouteToEmu(numPoints, totalDuration, fullResult, mode) {
        console.log("sendFullRouteToEmu(" + numPoints + ", " + totalDuration + ", " + fullResult + ", " + mode + ") faked");
    }

    saveRoute() {
        console.log("saveRoute() called");
    }

    onSavedRouteDrawn() {
        console.log("onSavedRouteDrawn() called");
    }
}
class MockObjects {
    constructor() {
        this.emulocationserver = new MockEmuLocationServer();
    }
}
class MockChannel {
    constructor() {
        this.objects = new MockObjects();
    }
}
// |channel| is only defined if running inside QtWebEngine.
if (typeof (channel) === 'undefined') {
    channel = new MockChannel();
}
