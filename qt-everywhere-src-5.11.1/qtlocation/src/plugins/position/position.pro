TEMPLATE = subdirs

QT_FOR_CONFIG += positioning-private

qtHaveModule(dbus):SUBDIRS += geoclue
qtConfig(gypsy):SUBDIRS += gypsy
qtConfig(winrt_geolocation):SUBDIRS += winrt
qtHaveModule(simulator):SUBDIRS += simulator
osx|ios|tvos:SUBDIRS += corelocation
android:SUBDIRS += android
qtHaveModule(serialport):SUBDIRS += serialnmea

SUBDIRS += \
    positionpoll
