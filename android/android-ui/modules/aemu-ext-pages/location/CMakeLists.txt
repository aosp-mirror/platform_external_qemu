if(QTWEBENGINE)
  message(STATUS "Building fancy location ui")
  android_qt_lib(
    TARGET ext-page-location-web
    ALIAS ext-page-location-ui
    INCLUDES
    src # Remove this once you are ready for shared libs. DLLEXPORT
        # LOCATION_EXPORTS DLLENABLE AEMU_LOCATION_SHARED SHARED
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page.ui
        android/skin/qt/extended-pages/location-route-playback-item.h
        android/skin/qt/extended-pages/network-connectivity-manager.cpp
        android/skin/qt/websockets/websocketclientwrapper.cpp
        android/skin/qt/websockets/websockettransport.cpp
    # We should move the resourcess here if this becomes a shared lib.
    DEPS android-emu
         android-emu-curl
         android-emu-location
         protobuf::libprotobuf
         qemu-host-common-headers
         Qt${QT_VERSION_MAJOR}::Network
         Qt${QT_VERSION_MAJOR}::WebChannel
         Qt${QT_VERSION_MAJOR}::WebEngineWidgets
         Qt${QT_VERSION_MAJOR}::WebSockets
         Qt${QT_VERSION_MAJOR}::Widgets
         ui::common
         ui::gl-qt-bridge
         ui::widgets)

  if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(ext-page-location-web
                          PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)
  endif()
  target_compile_definitions(ext-page-location-web PUBLIC USE_WEBENGINE)
else()
  message(STATUS "Building simple location ui")
  android_qt_lib(
    TARGET ext-page-location-no-web
    ALIAS ext-page-location-ui
    # Remove this once you are ready for shared libs. DLLEXPORT LOCATION_EXPORTS
    # DLLENABLE AEMU_LOCATION_SHARED SHARED
    INCLUDES
    src
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page_noMaps.ui
    DEPS android-emu
         android-emu-curl
         android-emu-location
         protobuf::libprotobuf
         qemu-host-common-headers
         Qt${QT_VERSION_MAJOR}::Core
         Qt${QT_VERSION_MAJOR}::Widgets
         ui::common
         ui::gl-qt-bridge
         ui::widgets)
endif()
