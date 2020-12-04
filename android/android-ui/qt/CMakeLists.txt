# The list of dependencies.
prebuilt(QT5)

set(ANDROID_UI_QT_MOC_SRC_FILES
    # cmake-format: sortable
    emulator-qt-window.h)

set(ANDROID_UI_QT_SRC_FILES
    # cmake-format: sortable
    emulator-controller-client.cpp
    emulator-qt-window.cpp
    main.cpp)
# UI Standalone -- Qt
android_add_executable(
  TARGET android_ui_qt
  NODISTRIBUTE
  SRC # cmake-format: sortable
      ${ANDROID_UI_QT_MOC_SRC_FILES}
      ${ANDROID_UI_QT_SRC_FILES})
target_link_libraries(android_ui_qt
                      PRIVATE Qt5::Core
                              Qt5::Widgets
                              android-grpc)
