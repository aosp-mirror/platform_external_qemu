class EventBus {
    constructor() {
        this.eventDispatchTable = {};
    }

    dispatch(eventName, eventData) {
        console.log('dispatching event', eventName, eventData);
        const eventEntry = this.eventDispatchTable[eventName];
        if (eventEntry) {
            eventEntry.fire(eventData);
        }
    }

    on(eventNameOrEventArray, eventListener) {
        let eventNames = new Array();
        if (typeof(eventNameOrEventArray) === 'string') {
            eventNames.push(eventNameOrEventArray);
        } else {
            eventNames = eventNameOrEventArray;
        }
        for (let i = 0; i < eventNames.length; i++) {
            const eventName = eventNames[i];
            let eventEntry = this.eventDispatchTable[eventName];
            if (!eventEntry) {
                eventEntry = new EventDispatchTableEntry(eventName);
                this.eventDispatchTable[eventName] = eventEntry;
            }
            eventEntry.addListener(eventListener);    
        }
    }

    off(eventName, eventListener) {
        const eventEntry = this.eventDispatchTable[eventName];
        if (eventEntry && eventEntry.listeners.indexOf(eventListener) > -1) {
            eventEntry.removeListener(eventListener);
            if (eventEntry.listeners.length === 0) {
                delete this.eventDispatchTable[eventName];
            }
        }
    }

}

class EventDispatchTableEntry {
    constructor(eventName) {
        this.eventName = eventName;
        this.listeners = [];
    }

    addListener(eventListener) {
        this.listeners.push(eventListener);
    }

    removeListener(eventListener) {
        const index = this.listeners.indexOf(eventListener);
        if (index > - 1) {
            this.listeners.splice(index, 1);
        }
    }

    fire(eventData) {
        const eventListeners = Array.from(this.listeners);
        for (let i = 0; i < eventListeners.length; i++) {
            eventListeners[i](eventData);
        }
    }
}