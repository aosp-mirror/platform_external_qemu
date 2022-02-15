/* Copyright (C) 2015-2017 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "android/skin/qt/emulator-qt-window.h"

#include "android/android.h"
#include "android/avd/info.h"
#include "android/base/Optional.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32Utils.h"
#include "android/base/threads/Async.h"
#include "android/cmdline-option.h"
#include "android/cpu_accelerator.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/hw-sensors.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/metrics.h"
#include "android/multitouch-screen.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/FramelessDetector.h"
#include "android/skin/qt/QtLooper.h"
#include "android/skin/qt/event-serializer.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/multi-display-page.h"
#include "android/skin/qt/extended-pages/settings-page.h"
#include "android/skin/qt/extended-pages/snapshot-page.h"
#include "android/skin/qt/extended-pages/telephony-page.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/screen-mask.h"
#include "android/skin/qt/winsys-qt.h"
#include "android/skin/rect.h"
#include "android/skin/winsys.h"
#include "android/snapshot/Snapshotter.h"
#include "android/snapshot/common.h"
#include "android/test/checkboot.h"
#include "android/ui-emu-agent.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/filelock.h"
#include "android/utils/x86_cpuid.h"
#include "android/virtualscene/TextureUtils.h"
#include "studio_stats.pb.h"
#include "android_modem_v2.h"

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"
#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

#include <Qt>
#include <QBitmap>
#include <QCheckBox>
#include <QCursor>
#if QT_VERSION >= 0x060000
#else
#include <QDesktopWidget>
#endif  // QT_VERSION
#include <QFileDialog>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSemaphore>
#include <QSettings>
#include <QTabletEvent>
#include <QToolTip>
#include <QTouchEvent>
#include <QWindow>
#include <QtCore>
#include <QtMath>

#ifdef _WIN32
#include <io.h>
#else
#ifndef _MSC_VER
#include <unistd.h>
#endif
#endif

#include <array>
#include <cmath>
#include <string>
#include <vector>

#ifdef __linux__
// This include needs to be after all the Qt includes
// because it defines macros/types that conflict with
// qt's own macros/types.
#elif defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#else  // windows
#include <windows.h>
#endif

using android::base::AutoLock;
using android::base::c_str;
using android::base::kNullopt;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::makeOptional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::crashreport::CrashReporter;
using android::emulation::ApkInstaller;
using android::emulation::FilePusher;
using android::virtualscene::TextureUtils;
using std::string;
using std::vector;

// Make sure it is POD here
static LazyInstance<EmulatorQtWindow::Ptr> sInstance = LAZY_INSTANCE_INIT;

// static
const int EmulatorQtWindow::kPushProgressBarMax = 2000;
const StringView EmulatorQtWindow::kRemoteDownloadsDir = "/sdcard/Download/";

constexpr Qt::WindowFlags EmulatorQtWindow::FRAMED_WINDOW_FLAGS;
constexpr Qt::WindowFlags EmulatorQtWindow::FRAMELESS_WINDOW_FLAGS;
constexpr Qt::WindowFlags EmulatorQtWindow::FRAME_WINDOW_FLAGS_MASK;

// Gingerbread devices download files to the following directory (note the
// uncapitalized "download").
const StringView EmulatorQtWindow::kRemoteDownloadsDirApi10 =
        "/sdcard/download/";

// Place to tell everyone we're exiting
//
// Isn't a member of the window object, so no issues with racing to delete
bool EmulatorQtWindow::sClosed = false;

SkinSurfaceBitmap::SkinSurfaceBitmap(int w, int h) : cachedSize(w, h) {}

SkinSurfaceBitmap::SkinSurfaceBitmap(const char* path) : reader(path) {
    cachedSize = reader.size();
    // When loading png file from a file path, QT's QImage class will use its
    // own libpng which introduces unexpected dependencies on libz from system
    // lib. Thus, causing crash for certain png file. (b/127953242) To
    // workaround, we use an inplementation which is based on libpng built from
    // source and it doesn't introduce unexpected dependencies.
    // This code path will be executed iff the file format is png.
    if (PathUtils::extension(path) == ".png") {
        auto result =
                TextureUtils::loadPNG(reader.fileName().toStdString().c_str(),
                                      TextureUtils::Orientation::Qt);
        if (result) {
            QImage::Format format =
                    (result->mFormat == TextureUtils::Format::RGB24)
                            ? QImage::Format_RGB888
                            : QImage::Format_RGBA8888;
            image = QImage(reinterpret_cast<const unsigned char*>(
                                   result->mBuffer.data()),
                           result->mWidth, result->mHeight, format);

            image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            resetReader();
        }
    }
}

SkinSurfaceBitmap::SkinSurfaceBitmap(const unsigned char* data, int size) {
    auto array =
            new QByteArray(QByteArray::fromRawData((const char*)data, size));
    auto buffer = new QBuffer(array);
    buffer->open(QIODevice::ReadOnly);
    reader.setDevice(buffer);
    cachedSize = reader.size();
}

SkinSurfaceBitmap::SkinSurfaceBitmap(const SkinSurfaceBitmap& other,
                                     int rotation,
                                     int blend)
    : image(other.image), cachedSize(other.cachedSize) {
    // Mix two alpha blend values to produce some average in the same 0..256
    // range as the original one. Round it up to minimize chances when we end up
    // with |pendingBlend| == 0.
    pendingBlend = (blend * other.pendingBlend + SKIN_BLEND_FULL - 1) /
                   SKIN_BLEND_FULL;
    pendingRotation = (rotation + other.pendingRotation) & 3;

    if (other.reader.device()) {
        if (other.reader.fileName().isEmpty()) {
            auto otherBuffer = qobject_cast<QBuffer*>(other.reader.device());
            auto array = new QByteArray(otherBuffer->buffer());
            auto buffer = new QBuffer(array);
            buffer->open(QIODevice::ReadOnly);
            reader.setDevice(buffer);
        } else {
            reader.setFileName(other.reader.fileName());
        }
    }
}

void SkinSurfaceBitmap::resetReader() {
    if (reader.device()) {
        if (auto buffer = qobject_cast<QBuffer*>(reader.device())) {
            delete &buffer->buffer();
            delete buffer;
        } else {
            reader.device()->moveToThread(QThread::currentThread());
        }
        reader.setDevice(nullptr);
    }
}

SkinSurfaceBitmap::~SkinSurfaceBitmap() {
    resetReader();
}

QSize SkinSurfaceBitmap::size() const {
    return (pendingRotation & 1) ? cachedSize.transposed() : cachedSize;
}

void SkinSurfaceBitmap::applyPendingTransformations() {
    if (!hasPendingTransformations()) {
        return;
    }

    readImage();
    if (pendingBlend == SKIN_BLEND_FULL) {
        if (pendingRotation == SKIN_ROTATION_180) {
            // can shortcut it with mirroring
            image = image.mirrored(true, true);
        } else {
            // a simple transformation is still enough here
#if QT_VERSION >= 0x060000
            QTransform transform;
#else
            QMatrix transform;
#endif
            transform.rotate(pendingRotation * 90);
            image = image.transformed(transform);
        }
    } else {
        QImage newImage(size(), QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&newImage);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.setOpacity(pendingBlend / double(SKIN_BLEND_FULL));
        painter.rotate(pendingRotation * 90);
        painter.drawImage(QPoint(), image);
        painter.end();
        image = newImage;
    }
    pendingBlend = SKIN_BLEND_FULL;
    pendingRotation = SKIN_ROTATION_0;
    cachedSize = image.size();
}

bool SkinSurfaceBitmap::hasPendingTransformations() const {
    return pendingRotation != SKIN_ROTATION_0 ||
           pendingBlend != SKIN_BLEND_FULL;
}

void SkinSurfaceBitmap::fill(const QRect& area, const QColor& color) {
    if (area.topLeft() == QPoint() && area.size() == size()) {
        // simple case - fill the whole image
        if (image.isNull() || (pendingRotation & 1)) {
            image = QImage(size(), QImage::Format_ARGB32_Premultiplied);
        }
        image.fill(color);
        resetReader();
        pendingBlend = SKIN_BLEND_FULL;
        pendingRotation = SKIN_ROTATION_0;
        cachedSize = image.size();
    } else {
        QPainter painter(&get());
        painter.fillRect(area, color);
    }
}

void SkinSurfaceBitmap::drawFrom(SkinSurfaceBitmap* what,
                                 QPoint where,
                                 const QRect& area,
                                 QPainter::CompositionMode op) {
    if (area.topLeft() == QPoint() && area.size() == size() &&
        what->get().size() == area.size() &&
        op == QPainter::CompositionMode_Source) {
        image = what->get();
        resetReader();
        pendingBlend = SKIN_BLEND_FULL;
        pendingRotation = SKIN_ROTATION_0;
        cachedSize = image.size();
    } else {
        QPainter painter(&get());
        painter.setCompositionMode(op);
        painter.drawImage(where, what->get(), area);
    }
}

QImage& SkinSurfaceBitmap::get() {
    readImage();
    applyPendingTransformations();
    return image;
}

void SkinSurfaceBitmap::readImage() {
    if (!image.isNull()) {
        return;
    }
    if (reader.device()) {
        reader.read(&image);
        if (image.format() != QImage::Format_ARGB32_Premultiplied) {
            image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        }
        resetReader();
    } else {
        image = QImage(cachedSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(0);
    }
}

////////////////////////////////////////////////////////////////////////////////

void EmulatorQtWindow::create() {
    sInstance.get() = Ptr(new EmulatorQtWindow());
}

EmulatorQtWindow::EmulatorQtWindow(QWidget* parent)
    : QFrame(parent),
      mLooper(android::qt::createLooper()),
      mStartupDialog(this),
      mToolWindow(nullptr),
      mCarClusterWindow(nullptr),
      mCarClusterConnector(nullptr),
      mContainer(this),
      mOverlay(this, &mContainer),
      mZoomFactor(1.0),
      mInZoomMode(false),
      mNextIsZoom(false),
      mForwardShortcutsToDevice(false),
      mPrevMousePosition(0, 0),
      mSkinGapTop(0),
      mSkinGapRight(0),
      mSkinGapBottom(0),
      mSkinGapLeft(0),
      mMainLoopThread(nullptr),
      mWin32WarningBox(
              QMessageBox::Information,
              tr("Windows 32-bit Deprecation"),
              tr("We strongly recommend using the 64-bit version "
                 "of the Windows emulator; the 32-bit version is deprecated "
                 "and will be removed soon."),
              QMessageBox::Ok,
              this),
      mAvdWarningBox(QMessageBox::Information,
                     tr("Recommended AVD"),
                     tr("Running an x86 based Android Virtual Device (AVD) is "
                        "10x faster.<br/>"
                        "We strongly recommend creating a new AVD."),
                     QMessageBox::Ok,
                     this),
      mGpuWarningBox([this] {
          return std::make_tuple(
                  QMessageBox::Information, tr("GPU Driver Issue"),
                  tr("Your GPU driver information:\n\n"
                     "%1\n"
                     "Some users have experienced emulator stability issues "
                     "with this driver version.  As a result, we're "
                     "selecting a compatibility renderer.  Please check with "
                     "your manufacturer to see if there is an updated "
                     "driver available.")
                          .arg(QString::fromStdString(
                                  globalGpuInfoList().dump())),
                  QMessageBox::Ok, this);
      }),
      mAdbWarningBox(QMessageBox::Warning,
                     tr("Detected ADB"),
                     tr(""),
                     QMessageBox::Ok,
                     this),
#ifdef _WIN32
      mVgkWarningBox(QMessageBox::Information,
                     tr("Incompatible Software Detected"),
                     tr("Vanguard anti-cheat software is deteced on your "
                        "system. It is known to have compatibility issues "
                        "with Android emulator. It is recommended to uninstall "
                        "or deactivate Vanguard anti-cheat software while "
                        "running Android emulator."),
                     QMessageBox::Ok,
                     this),
#endif
      mNestedWarningBox(QMessageBox::Information,
                     tr("Emulator Running in Nested Virtualization"),
                     tr("Emulator is running using nested virtualization. "
                        "This is not recommended. It may not work at all. "
                        "And typically the performance is not quite good."),
                     QMessageBox::Ok,
                     this),
      mEventLogger(
              std::make_shared<UIEventRecorder<android::base::CircularBuffer>>(
                      &mEventCapturer,
                      1000)),
      mUserActionsCounter(new android::qt::UserActionsCounter(&mEventCapturer)),
      mAdbInterface([this] {
          return android::emulation::AdbInterface::createGlobalOwnThread();
      }),
      mApkInstaller([this] { return (*mAdbInterface); }),
      mFilePusher([this] {
          return std::make_tuple(
                  (*mAdbInterface),
                  [this](StringView filePath, FilePusher::Result result) {
                      runOnUiThread([this, filePath, result] {
                        adbPushDone(filePath, result);
                      });
                  },
                  [this](double progress, bool done) {
                      runOnUiThread([this, progress, done] {
                        adbPushProgress(progress, done);
                      });
                  });
      }),
      mInstallDialog(this,
                     [this](QProgressDialog* dialog) {
                         dialog->setWindowTitle(tr("APK Installer"));
                         dialog->setLabelText(tr("Installing APK..."));
                         dialog->setRange(0, 0);  // Makes it a "busy" dialog
                         dialog->setModal(true);
                         dialog->close();
                         QObject::connect(dialog, SIGNAL(canceled()), this,
                                          SLOT(slot_installCanceled()));
                     }),
      mPushDialog(this,
                  [this](QProgressDialog* dialog) {
                      dialog->setWindowTitle(tr("File Copy"));
                      dialog->setLabelText(tr("Copying files..."));
                      dialog->setRange(0, kPushProgressBarMax);
                      dialog->close();
                      QObject::connect(dialog, SIGNAL(canceled()), this,
                                       SLOT(slot_adbPushCanceled()));
                  }),
      mStartedAdbStopProcess(false),
      mHaveBeenFrameless(false) {
    qRegisterMetaType<QPainter::CompositionMode>();
    qRegisterMetaType<SkinRotation>();
    qRegisterMetaType<SkinGenericFunction>();
    qRegisterMetaType<RunOnUiThreadFunc>();
    qRegisterMetaType<Ui::OverlayMessageType>();
    qRegisterMetaType<Ui::OverlayChildWidget::DismissFunc>();

    mOrientation = !strcmp(android_hw->hw_initialOrientation, "landscape") ?
                       SKIN_ROTATION_270 : SKIN_ROTATION_0;

    android::base::ThreadLooper::setLooper(mLooper, true);

    // TODO: Weird input lag with Qt will cause hang detector to trip and crash
    // the emulator, even though it works ok (but laggy) otherwise.
    // For now, disable Qt hang detection.
    // bug: 73723222
    // bug: 74520987
    // CrashReporter::get()->hangDetector().addWatchedLooper(mLooper);

    // Start a timer. If the main window doesn't
    // appear before the timer expires, show a
    // pop-up to let the user know we're still
    // working.
    QObject::connect(&mStartupTimer, &QTimer::timeout, this,
                     &EmulatorQtWindow::slot_startupTick);
    mStartupTimer.setSingleShot(true);
    mStartupTimer.setInterval(500);  // Half a second
    mStartupTimer.start();

    mBackingSurface = NULL;

    QSettings settings;
    mFrameAlways =
        FramelessDetector::isFramelessOk() ?
            settings.value(Ui::Settings::FRAME_ALWAYS, false).toBool() : true;
    mPreviouslyFramed = mFrameAlways;

    mToolWindow = new ToolWindow(this, &mContainer, mEventLogger,
                                 mUserActionsCounter);

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        mCarClusterWindow = new CarClusterWindow(this, &mContainer);
        mCarClusterConnector = new CarClusterConnector(mCarClusterWindow);
    }

    this->setAcceptDrops(true);
    QObject::connect(this, &EmulatorQtWindow::showVirtualSceneControls,
                     mToolWindow, &ToolWindow::showVirtualSceneControls);

    QObject::connect(this, &EmulatorQtWindow::blit, this,
                     &EmulatorQtWindow::slot_blit);
    QObject::connect(this, &EmulatorQtWindow::fill, this,
                     &EmulatorQtWindow::slot_fill);
    QObject::connect(this, &EmulatorQtWindow::getDevicePixelRatio, this,
                     &EmulatorQtWindow::slot_getDevicePixelRatio);
    QObject::connect(this, &EmulatorQtWindow::getScreenDimensions, this,
                     &EmulatorQtWindow::slot_getScreenDimensions);
    QObject::connect(this, &EmulatorQtWindow::getFramePos, this,
                     &EmulatorQtWindow::slot_getFramePos);
    QObject::connect(this, &EmulatorQtWindow::windowHasFrame, this,
                     &EmulatorQtWindow::slot_windowHasFrame);
    QObject::connect(this, &EmulatorQtWindow::getWindowPos, this,
                     &EmulatorQtWindow::slot_getWindowPos);
    QObject::connect(this, &EmulatorQtWindow::getWindowSize, this,
                     &EmulatorQtWindow::slot_getWindowSize);
    QObject::connect(this, &EmulatorQtWindow::isWindowFullyVisible, this,
                     &EmulatorQtWindow::slot_isWindowFullyVisible);
    QObject::connect(this, &EmulatorQtWindow::isWindowOffScreen, this,
                     &EmulatorQtWindow::slot_isWindowOffScreen);
    QObject::connect(this, &EmulatorQtWindow::releaseBitmap, this,
                     &EmulatorQtWindow::slot_releaseBitmap);
    QObject::connect(this, &EmulatorQtWindow::requestClose, this,
                     &EmulatorQtWindow::slot_requestClose);
    QObject::connect(this, &EmulatorQtWindow::requestUpdate, this,
                     &EmulatorQtWindow::slot_requestUpdate);
    QObject::connect(this, &EmulatorQtWindow::setDeviceGeometry, this,
                     &EmulatorQtWindow::slot_setDeviceGeometry);
    QObject::connect(this, &EmulatorQtWindow::setWindowIcon, this,
                     &EmulatorQtWindow::slot_setWindowIcon, Qt::QueuedConnection);
    QObject::connect(this, &EmulatorQtWindow::setWindowPos, this,
                     &EmulatorQtWindow::slot_setWindowPos);
    QObject::connect(this, &EmulatorQtWindow::setWindowSize, this,
                     &EmulatorQtWindow::slot_setWindowSize);
    QObject::connect(this, &EmulatorQtWindow::paintWindowOverlayForResize, this,
                     &EmulatorQtWindow::slot_paintWindowOverlayForResize);
    QObject::connect(this, &EmulatorQtWindow::setWindowOverlayForResize, this,
                     &EmulatorQtWindow::slot_setWindowOverlayForResize);
    QObject::connect(this, &EmulatorQtWindow::clearWindowOverlay, this,
                     &EmulatorQtWindow::slot_clearWindowOverlay);
    QObject::connect(this, &EmulatorQtWindow::setWindowCursorResize, this,
                     &EmulatorQtWindow::slot_setWindowCursorResize);
    QObject::connect(this, &EmulatorQtWindow::setWindowCursorNormal, this,
                     &EmulatorQtWindow::slot_setWindowCursorNormal);
    QObject::connect(this, &EmulatorQtWindow::setTitle, this,
                     &EmulatorQtWindow::slot_setWindowTitle);
    QObject::connect(this, &EmulatorQtWindow::showWindow, this,
                     &EmulatorQtWindow::slot_showWindow);
    QObject::connect(this, &EmulatorQtWindow::updateRotation, this,
                     &EmulatorQtWindow::slot_updateRotation);
    QObject::connect(this, &EmulatorQtWindow::runOnUiThread, this,
                     &EmulatorQtWindow::slot_runOnUiThread);
    QObject::connect(this, &EmulatorQtWindow::showMessage, this,
                     &EmulatorQtWindow::slot_showMessage);
    QObject::connect(this, &EmulatorQtWindow::showMessageWithDismissCallback, this,
                     &EmulatorQtWindow::slot_showMessageWithDismissCallback);
    QObject::connect(QApplication::instance(), &QCoreApplication::aboutToQuit,
                     this, &EmulatorQtWindow::slot_clearInstance);

    QObject::connect(mContainer.horizontalScrollBar(),
                     SIGNAL(valueChanged(int)), this,
                     SLOT(slot_horizontalScrollChanged(int)));
    QObject::connect(mContainer.verticalScrollBar(), SIGNAL(valueChanged(int)),
                     this, SLOT(slot_verticalScrollChanged(int)));
    QObject::connect(mContainer.horizontalScrollBar(),
                     SIGNAL(rangeChanged(int, int)), this,
                     SLOT(slot_scrollRangeChanged(int, int)));
    QObject::connect(mContainer.verticalScrollBar(),
                     SIGNAL(rangeChanged(int, int)), this,
                     SLOT(slot_scrollRangeChanged(int, int)));

    bool onTop = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    setOnTop(onTop);

    // Our paintEvent() always fills the whole window.
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_AcceptTouchEvents);

    bool shortcutBool =
            settings.value(Ui::Settings::FORWARD_SHORTCUTS_TO_DEVICE, false)
                    .toBool();
    setForwardShortcutsToDevice(shortcutBool ? 1 : 0);

    initErrorDialog(this);
    setObjectName("MainWindow");
    mEventLogger->startRecording(this);
    mEventLogger->startRecording(mToolWindow);
    mEventLogger->startRecording(&mOverlay);
    mUserActionsCounter->startCountingForMainWindow(this);
    mUserActionsCounter->startCountingForToolWindow(mToolWindow);
    mUserActionsCounter->startCountingForOverlayWindow(&mOverlay);

    // The crash reporter will dump the last X UI events.
    // mEventLogger is a shared pointer, capturing its copy inside a lambda
    // ensures that it lives on as long as CrashReporter needs it, even if
    // EmulatorQtWindow is destroyed.
    auto eventLogger = mEventLogger;
    CrashReporter::get()->addCrashCallback([eventLogger]() {
            eventLogger->stop();
            auto fd = CrashReporter::get()->openDataAttachFile(
                          "recent-ui-actions.txt");
            if (fd.valid()) {
                const auto& events = eventLogger->container();
                for (int i = 0; i < events.size(); ++i) {
                    HANDLE_EINTR(write(fd.get(), events[i].data(),
                                       events[i].size()));
                    HANDLE_EINTR(write(fd.get(), "\n", 1));
                }
            }
    });

    auto userActions = mUserActionsCounter;
    CrashReporter::get()->addCrashCallback([userActions]() {
        char actions[16] = {};
        snprintf(actions, sizeof(actions) - 1, "%" PRIu64,
                 userActions->count());
        char filename[32 + sizeof(actions)] = {};
        snprintf(filename, sizeof(filename) - 1, "num-user-actions-%s.txt",
                 actions);
        CrashReporter::get()->attachData(filename, actions);
    });

    std::weak_ptr<android::qt::UserActionsCounter> userActionsWeak(
            mUserActionsCounter);
    using android::metrics::PeriodicReporter;
    mMetricsReportingToken = PeriodicReporter::get().addCancelableTask(
            60 * 10 * 1000,  // reporting period
            [userActionsWeak](android_studio::AndroidStudioEvent* event) {
                if (auto user_actions = userActionsWeak.lock()) {
                    const auto actionsCount = user_actions->count();
                    event->mutable_emulator_ui_event()->set_context(
                            android_studio::EmulatorUiEvent::
                                    UNKNOWN_EMULATOR_UI_EVENT_CONTEXT);
                    event->mutable_emulator_ui_event()->set_type(
                            android_studio::EmulatorUiEvent::
                                    UNKONWN_EMULATOR_UI_EVENT_TYPE);
                    event->mutable_emulator_ui_event()->set_value(
                            actionsCount);
                    event->set_kind(
                            android_studio::AndroidStudioEvent::
                                    EMULATOR_UI_EVENT);
                    return true;
                }
                return false;
            });

    setFrameAlways(mFrameAlways);

    mWheelScrollTimer.setInterval(100);
    mWheelScrollTimer.setSingleShot(true);
    connect(&mWheelScrollTimer, SIGNAL(timeout()), this,
            SLOT(wheelScrollTimeout()));

    mIgnoreWheelEvent =
        settings.value(Ui::Settings::DISABLE_MOUSE_WHEEL, false).toBool();

    // set custom ADB path if saved
    bool autoFindAdb =
            settings.value(Ui::Settings::AUTO_FIND_ADB, true).toBool();
    if (!autoFindAdb) {
        QString adbPath = settings.value(Ui::Settings::ADB_PATH, "").toString();
        if (!adbPath.isEmpty()) {
            adbPath = QDir::toNativeSeparators(adbPath);
            (*mAdbInterface)->setCustomAdbPath(adbPath.toStdString());
        }
    }


    mPauseAvdWhenMinimized = SettingsPage::getPauseAvdWhenMinimized();

    ScreenMask::loadMask();

    using android::snapshot::Snapshotter;
    Snapshotter::get().addOperationCallback(
            [this](Snapshotter::Operation op, Snapshotter::Stage stage) {
                if (stage == Snapshotter::Stage::Start) {
                    runOnUiThread([this, op]() {
                        AutoLock lock(mSnapshotStateLock);
                        mShouldShowSnapshotModalOverlay = true;
                        QTimer::singleShot(500, QApplication::instance(),
                            [this, op]() {
                                AutoLock lock(mSnapshotStateLock);
                                if (mShouldShowSnapshotModalOverlay) {
                                    mContainer.showModalOverlay(
                                        op == Snapshotter::Operation::Save ?
                                            tr("Saving state...") :
                                            tr("Loading state..."));
                                    if (op == Snapshotter::Operation::Save) {
                                        mContainer.setModalOverlayFunc(
                                                tr("CANCEL"), [] {
                                                    androidSnapshot_cancelSave();
                                                });
                                    }
                                }
                            });
                        if (mToolWindow) {
                            mToolWindow->setEnabled(false);
                        }
                        if (mCarClusterWindow) {
                            mCarClusterWindow->setEnabled(false);
                        }
                        if (SnapshotPage::get()) {
                            SnapshotPage::get()->setOperationInProgress(true);
                        }
                    });
                } else if (stage == Snapshotter::Stage::End) {
                    runOnUiThread([this, op]() {
                        AutoLock lock(mSnapshotStateLock);
                        mShouldShowSnapshotModalOverlay = false;
                        mContainer.hideModalOverlay();
                        if (mToolWindow) {
                            mToolWindow->setEnabled(true);
                        }
                        if (mCarClusterWindow) {
                            mCarClusterWindow->setEnabled(true);
                        }
                        if (SnapshotPage::get()) {
                            SnapshotPage::get()->setOperationInProgress(false);
                        }
                    });
                }
            });
}

EmulatorQtWindow::Ptr EmulatorQtWindow::getInstancePtr() {
    return sInstance.get();
}

EmulatorQtWindow* EmulatorQtWindow::getInstance() {
    return getInstancePtr().get();
}

EmulatorQtWindow::~EmulatorQtWindow() {
    if (mApkInstallCommand) {
        mApkInstallCommand->cancel();
    }
    mInstallDialog.clear();
    mPushDialog.clear();

    deleteErrorDialog();
    if (mToolWindow) {
        delete mToolWindow;
        mToolWindow = NULL;
    }

    if (mCarClusterWindow) {
        delete mCarClusterWindow;
        mCarClusterWindow = NULL;
    }

    if (mCarClusterConnector) {
        delete mCarClusterConnector;
        mCarClusterConnector = NULL;
    }

    mStartupDialog.ifExists([&] {
        mStartupDialog->hide();
        mStartupDialog.clear();
    });

    AutoLock lock(mSnapshotStateLock);
    mShouldShowSnapshotModalOverlay = false;
    mContainer.hideModalOverlay();
    lock.unlock();

    delete mMainLoopThread;
}

void EmulatorQtWindow::showWin32DeprecationWarning() {
#ifdef _WIN32
    auto programBitness = System::get()->getProgramBitness();
    if (programBitness != 32) return;

    mWin32WarningBox->show();
#endif
}

void EmulatorQtWindow::showAvdArchWarning() {
    ScopedCPtr<char> arch(avdInfo_getTargetCpuArch(android_avdInfo));

    // On Apple, we could also be running w/ Virtualization.framework
    // which should also support fast x86 VMs on arm64.
#if defined(__APPLE__) || defined (__x86_64__)
    if (!strcmp(arch.get(), "x86") || !strcmp(arch.get(), "x86_64")) {
        return;
    }
#endif

#ifdef __aarch64__
    if (!strcmp(arch.get(), "arm64")) {
        return;
    }
#endif

    // The following statuses indicate that the machine hardware does not
    // support hardware acceleration. These machines should never show a
    // popup indicating to switch to x86.
    static const AndroidCpuAcceleration badStatuses[] = {
            ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED,  // HAXM doesn't
                                                            // support
                                                            // nested VM
            ANDROID_CPU_ACCELERATION_INTEL_REQUIRED,        // HAXM requires
                                                            // GeniuneIntel
                                                            // processor
            ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT,  // CPU doesn't support
                                                      // required
                                                      // features (VT-x or SVM)
            ANDROID_CPU_ACCELERATION_NO_CPU_VTX_SUPPORT,  // CPU doesn't support
                                                          // VT-x
            ANDROID_CPU_ACCELERATION_NO_CPU_NX_SUPPORT,   // CPU doesn't support
                                                          // NX
    };

    AndroidCpuAcceleration cpuStatus =
            androidCpuAcceleration_getStatus(nullptr);
    for (AndroidCpuAcceleration status : badStatuses) {
        if (cpuStatus == status) {
            return;
        }
    }

    QSettings settings;
    if (settings.value(Ui::Settings::SHOW_AVD_ARCH_WARNING, true).toBool()) {
        QObject::connect(mAvdWarningBox.ptr(),
                         SIGNAL(buttonClicked(QAbstractButton*)), this,
                         SLOT(slot_avdArchWarningMessageAccepted()));

        QCheckBox* checkbox = new QCheckBox(tr("Never show this again."));
        checkbox->setCheckState(Qt::Unchecked);
        mAvdWarningBox->setWindowModality(Qt::NonModal);
        mAvdWarningBox->setCheckBox(checkbox);
        mAvdWarningBox->show();
    }
}

void EmulatorQtWindow::checkShouldShowGpuWarning() {
    mGpuBlacklisted = globalGpuInfoList().blacklist_status;
    std::array<android::featurecontrol::Feature, 2> forced = {
        android::featurecontrol::ForceANGLE,
        android::featurecontrol::ForceSwiftshader,
    };

    for (const auto& elt : forced) {
        mHasForcedRenderer |= android::featurecontrol::isEnabled(elt);
    }

    mShouldShowGpuWarning =
        mGpuBlacklisted ||
        mHasForcedRenderer;
}

void EmulatorQtWindow::showGpuWarning() {

    if (!mShouldShowGpuWarning) {
        return;
    }

    QSettings settings;
    if (settings.value(Ui::Settings::SHOW_GPU_WARNING, true).toBool()) {
        QObject::connect(mGpuWarningBox.ptr(),
                         SIGNAL(buttonClicked(QAbstractButton*)), this,
                         SLOT(slot_gpuWarningMessageAccepted()));

        QCheckBox* checkbox = new QCheckBox(tr("Never show this again."));
        checkbox->setCheckState(Qt::Unchecked);
        mGpuWarningBox->setWindowModality(Qt::NonModal);
        mGpuWarningBox->setCheckBox(checkbox);
        mGpuWarningBox->show();
    }
}

void EmulatorQtWindow::slot_startupTick() {
    // It's been a while since we were launched, and the main
    // window still hasn't appeared.
    // Show a pop-up that lets the user know we are working.
    if (android_cmdLineOptions->qt_hide_window) {
        return;
    }
    if (is_fuchsia) {
        mStartupDialog->setWindowTitle(tr("Fuchsia Emulator"));
    } else {
        mStartupDialog->setWindowTitle(tr("Android Emulator"));
    }
    // Hide close/minimize/maximize buttons
    mStartupDialog->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint |
                                   Qt::WindowTitleHint);
    // Make sure the icon is the same as in the main window
    mStartupDialog->setWindowIcon(QApplication::windowIcon());

    // Emulator logo
    QLabel* label = new QLabel();
    label->setAlignment(Qt::AlignCenter);
    QSize size;
    size.setWidth(mStartupDialog->size().width() / 2);
    size.setHeight(size.width());
    QPixmap pixmap = windowIcon().pixmap(size);
    label->setPixmap(pixmap);
    mStartupDialog->setLabel(label);

    // The default progress bar on Windows isn't centered for some reason
    QProgressBar* bar = new QProgressBar();
    bar->setAlignment(Qt::AlignHCenter);
    mStartupDialog->setBar(bar);

    mStartupDialog->setRange(0, 0);      // Don't show % complete
    mStartupDialog->setCancelButton(0);  // No "cancel" button
    mStartupDialog->setMinimumSize(mStartupDialog->sizeHint());
    mStartupDialog->show();
}

void EmulatorQtWindow::slot_avdArchWarningMessageAccepted() {
    QCheckBox* checkbox = mAvdWarningBox->checkBox();
    if (checkbox->checkState() == Qt::Checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_AVD_ARCH_WARNING, false);
    }
}

void EmulatorQtWindow::slot_gpuWarningMessageAccepted() {
    QCheckBox* checkbox = mGpuWarningBox->checkBox();
    if (checkbox->checkState() == Qt::Checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_GPU_WARNING, false);
    }
}

void EmulatorQtWindow::closeEvent(QCloseEvent* event) {
    if (!mToolWindow->shouldClose()) {
        event->ignore();
        return;
    }

    if (mToolWindow) {
        mToolWindow->setEnabled(false);
    }

    if (mCarClusterWindow) {
        mCarClusterWindow->setEnabled(false);
    }

    for (auto const& iter : mMultiDisplayWindow) {
        if (iter.second != nullptr)
            iter.second->setEnabled(false);
    }

    saveMultidisplayToConfig();

    const bool alreadyClosed = mClosed;
    mClosed = true;
    sClosed = true;
    crashhandler_exitmode(__FUNCTION__);

    // Make sure we cancel everything related to startup dialog here, otherwise
    // it could remain as the only emulator's window and would keep it running
    // forever.
    mStartupTimer.stop();
    mStartupTimer.disconnect();
    mStartupDialog.ifExists([&] {
        mStartupDialog->hide();
        mStartupDialog.clear();
    });

    if (mMainLoopThread && mMainLoopThread->isRunning()) {
        if (!alreadyClosed) {
            // we dont want to restore to a state where the
            // framework is shut down by 'adb reboot -p'
            // so skip that step when saving vm on exit
            const bool fastSnapshotV1 =
                    android::featurecontrol::isEnabled(
                            android::featurecontrol::FastSnapshotV1) &&
                    emuglConfig_current_renderer_supports_snapshot();
            if (fastSnapshotV1) {
                // Tell the system that we are in saving; create a file lock.
                auto snapshotLockFilePath =
                    avdInfo_getSnapshotLockFilePath(android_avdInfo);
                if (snapshotLockFilePath &&
                    !filelock_create(
                        snapshotLockFilePath)) {
                    derror("unable to lock snapshot save on exit!\n");
                }
            }

            if (fastSnapshotV1 || savevm_on_exit) {
                queueQuitEvent();
            } else {
                if (android_hw->hw_arc) {
                    // Send power key event to guest.
                    // After 10 seconds, we force close it.
                    mToolWindow->forwardKeyToEmulator(LINUX_KEY_POWER, true);
                    android::base::ThreadLooper::get()->createTimer(
                        [](void* opaque, android::base::Looper::Timer* timer) {
                           static_cast<EmulatorQtWindow*>(opaque)->queueQuitEvent();
                        }, this)->startRelative(10000);
                } else {
                    runAdbShellPowerDownAndQuit();
                }
            }
        }
        event->ignore();
    } else {
        mContainer.hideModalOverlay();

        if (mToolWindow) {
            mToolWindow->hide();
            mToolWindow->closeExtendedWindow();
        }
        if (mCarClusterWindow) {
            mCarClusterWindow->hide();
        }
        for (auto const& iter : mMultiDisplayWindow) {
            if (iter.second != nullptr)
                iter.second->hide();
        }
        mContainer.hide();
        mOverlay.hide();
        hide();
        event->accept();
    }
}

void EmulatorQtWindow::queueQuitEvent() {
    queueSkinEvent(createSkinEvent(kEventQuit));
}

void EmulatorQtWindow::dragEnterEvent(QDragEnterEvent* event) {
    // Accept all drag enter events with any URL, then filter more in drop
    // events
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void EmulatorQtWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    QToolTip::hideText();
}

void EmulatorQtWindow::dragMoveEvent(QDragMoveEvent* event) {
    QToolTip::showText(mapToGlobal(event->pos()),
                       tr("APK's will be installed.<br/>"
                          "Other files will be copied to /sdcard/Download."));
}

void EmulatorQtWindow::dropEvent(QDropEvent* event) {
    // Modal dialogs don't prevent drag-and-drop! Manually check for a modal
    // dialog, and if so, reject the event.
    if (QApplication::activeModalWidget() != nullptr) {
        event->ignore();
        return;
    }

    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.length() == 0) {
        showErrorDialog(
                tr("Dropped content didn't have any files. Emulator only "
                   "supports drag-and-drop for files to copy them or install "
                   "a single APK."),
                tr("Drag and Drop"));
        return;
    }

    // Get the first url - if it's an APK and the only file, attempt to install
    // it
    QString url = urls[0].toLocalFile();
    if (url.endsWith(".apk") && urls.length() == 1) {
        runAdbInstall(url);
        return;
    } else {
        // If any of the files is an APK, intent was ambiguous
        for (int i = 0; i < urls.length(); i++) {
            if (urls[i].path().endsWith(".apk")) {
                showErrorDialog(
                        tr("Drag-and-drop can either install a single APK"
                           " file or copy one or more non-APK files to the"
                           " Emulator SD card."),
                        tr("Drag and Drop"));
                return;
            }
        }
        runAdbPush(urls);
    }
}

void EmulatorQtWindow::keyPressEvent(QKeyEvent* event) {
    handleKeyEvent(kEventKeyDown, event);
}

void EmulatorQtWindow::keyReleaseEvent(QKeyEvent* event) {
    handleKeyEvent(kEventKeyUp, event);

    // If we enabled trackball mode, tell Qt to always forward mouse movement
    // events. Otherwise, Qt will forward them only when a button is pressed.
    EmulatorWindow* ew = emulator_window_get();
    bool trackballActive = skin_ui_is_trackball_active(ew->ui);
    if (trackballActive != hasMouseTracking()) {
        setMouseTracking(trackballActive);
    }
}
void EmulatorQtWindow::mouseMoveEvent(QMouseEvent* event) {
    if (android::featurecontrol::isEnabled(
                android::featurecontrol::VirtioMouse) &&
        !android_cmdLineOptions->no_mouse_reposition) {

        // Block all the incoming mouse events if mouse is being moved to
        // center of the screen.
        if (!mMouseRepositioning &&
                        (event->source() == Qt::MouseEventNotSynthesized)) {
            // Pen long press generates synthesized mouse events,
            // which need to be filtered out
            SkinEventType eventType = translateMouseEventType(
                    kEventMouseMotion, event->button(), event->buttons());
            handleMouseEvent(eventType, getSkinMouseButton(event),
                             event->pos(), event->globalPos());
        }

        if (mMouseGrabbed) {
            QRect mouseRect(mContainer.rect().center() -
                                    QPoint(mContainer.rect().width() * 0.35f,
                                           mContainer.rect().height() * 0.35f),
                            mContainer.rect().center() +
                                    QPoint(mContainer.rect().width() * 0.35f,
                                           mContainer.rect().height() * 0.35f));
            // There may be multiple events to be handled when the mouse is
            // repositioned to the center of the window, thus we block all the
            // incoming mouseMoveEvents until the mouse position is completely
            // within the |mouseRect|.
            if (!mouseRect.contains(event->pos())) {
                if (!mMouseRepositioning) {
                    mMouseRepositioning = true;
                    QCursor::setPos(
                            mContainer.mapToGlobal(mContainer.rect().center()));
                    mPrevMousePosition = mContainer.rect().center();
                }
            } else {
                mMouseRepositioning = false;
            }
        }
    } else {
        // Pen long press generates synthesized mouse events,
        // which need to be filtered out
        if (event->source() == Qt::MouseEventNotSynthesized) {
            SkinEventType eventType = translateMouseEventType(
                    kEventMouseMotion, event->button(), event->buttons());
            handleMouseEvent(eventType, getSkinMouseButton(event),
                             event->pos(), event->globalPos());
        }
    }
}

void EmulatorQtWindow::leaveEvent(QEvent* event) {
    // On Mac, the cursor retains its shape even after it
    // leaves, if it is over our transparent region. Make
    // sure we're not showing the resize cursor. We can't
    // resize because we don't get events after the cursor
    // has left.
    mContainer.setCursor(Qt::ArrowCursor);
}

void EmulatorQtWindow::mousePressEvent(QMouseEvent* event) {
    if (android::featurecontrol::isEnabled(
                android::featurecontrol::VirtioMouse) &&
        !mMouseGrabbed) {
        if (mPromptMouseRestoreMessageBox) {
            QMessageBox msgBox;
            QString text =
                    "You have clicked the mouse inside the Emulator "
                    "display. This will cause Emulator to capture the "
                    "host mouse pointer and the keyboard, which will "
                    "make them unavailable to other host applications."
                    "<br><br>You can press "
#ifdef __APPLE__
                    "Cmd+R"
#else
                    "Ctrl+R"
#endif
                    " to uncapture the keyboard and "
                    "mouse and return to normal operation.";

            if (!android_cmdLineOptions->no_mouse_reposition) {
                text.append(
                        "<br><br>On remote desktops, mouse might not work "
                        "properly. Please enable relative mouse mode on "
                        "your remote desktop software, or add "
                        "\"--no-mouse-reposition\" argument to your "
                        "emulator arguments.");
            }
            msgBox.setText(text);
            msgBox.setTextFormat(Qt::TextFormat::AutoText);
            msgBox.exec();
            mPromptMouseRestoreMessageBox = false;
        }

        D("%s: mouse grabbed\n", __func__);
        grabMouse(QCursor(Qt::CursorShape::BlankCursor));

        SkinEvent* skin_event = createSkinEvent(kEventMouseStartTracking);
        queueSkinEvent(skin_event);

        mMouseGrabbed = true;
    }

    // Pen long press generates synthesized mouse events,
    // which need to be filtered out
    if (event->source() == Qt::MouseEventNotSynthesized) {
        SkinEventType eventType = translateMouseEventType(
                    kEventMouseButtonDown, event->button(), event->buttons());
        handleMouseEvent(eventType, getSkinMouseButton(event),
                         event->pos(), event->globalPos());
    }
}

void EmulatorQtWindow::mouseReleaseEvent(QMouseEvent* event) {
    // Pen long press generates synthesized mouse events,
    // which need to be filtered out
    if (event->source() == Qt::MouseEventNotSynthesized) {
        SkinEventType eventType = translateMouseEventType(
                    kEventMouseButtonUp, event->button(), event->buttons());
        handleMouseEvent(eventType, getSkinMouseButton(event),
                         event->pos(), event->globalPos());
    }
}

// Event handler for pen events as defined by Qt
void EmulatorQtWindow::tabletEvent(QTabletEvent* event) {
    SkinEventType eventType = kEventPenRelease;
    bool button_pressed = false;

    switch (event->type()) {
        case QEvent::TabletPress:
        {
            button_pressed =
                    ((event->buttons() & Qt::RightButton) == Qt::RightButton);
            eventType = translatePenEventType(kEventPenPress,
                                        event->button(), event->buttons());
            handlePenEvent(eventType, event);
            mPenPosX = event->pos().x();
            mPenPosY = event->pos().y();
            mPenButtonPressed = button_pressed;
            break;
        }
        case QEvent::TabletRelease:
        {
            button_pressed =
                    ((event->buttons() & Qt::RightButton) == Qt::RightButton);
            eventType = translatePenEventType(kEventPenRelease,
                                        event->button(), event->buttons());
            handlePenEvent(eventType, event);
            mPenPosX = event->pos().x();
            mPenPosY = event->pos().y();
            mPenButtonPressed = button_pressed;
            break;
        }
        case QEvent::TabletMove:
        {
            button_pressed =
                    ((event->buttons() & Qt::RightButton) == Qt::RightButton);
            if ((event->pos().x() != mPenPosX) ||
                (event->pos().y() != mPenPosY) ||
                (button_pressed   != mPenButtonPressed)) {
                eventType = translatePenEventType(kEventPenMove,
                                        event->button(), event->buttons());
                handlePenEvent(eventType, event);
                mPenPosX = event->pos().x();
                mPenPosY = event->pos().y();
                mPenButtonPressed = button_pressed;
            }
            break;
        }
        default:
            break;
    }
    event->accept();
}

// Set the window flags based on whether we should
// have a frame or not.
// Mask off the background of the skin PNG if we
// are running frameless.

void EmulatorQtWindow::maskWindowFrame() {
    bool haveFrame = hasFrame();
    Qt::WindowFlags flags = mContainer.windowFlags();

    flags &= ~FRAME_WINDOW_FLAGS_MASK;
    flags |= (haveFrame ? FRAMED_WINDOW_FLAGS : FRAMELESS_WINDOW_FLAGS);

    mContainer.setWindowFlags(flags);

    // Re-generate and apply the mask
    if (haveFrame) {
        // We have a frame. Do not use a mask around the device.
#ifdef _WIN32
        mContainer.clearMask();
#else
        // On Linux and Mac, clearMask() doesn't seem to work,
        // so we create a full rectangular mask. (Which doesn't
        // work right on Windows!)
        QRegion fullRegion(0, 0, mContainer.width(), mContainer.height());
        mContainer.setMask(fullRegion);

        if (!mHaveBeenFrameless) {
            // This is necessary on Mac. Doesn't hurt on Linux.
            mContainer.clearMask();
        }
#endif
        // We have a frame. There is no transparent gap around the skin.
        mSkinGapTop = 0;
        mSkinGapRight = 0;
        mSkinGapBottom = 0;
        mSkinGapLeft = 0;
    } else {
        // Frameless: Do an intelligent mask.
        // Start by reloading the skin PNG file.
        mHaveBeenFrameless = true;

        getSkinPixmap();
        if (mRawSkinPixmap != nullptr) {
            // Rotate the skin to match the emulator window
            QTransform rotater;
            // Set the native rotation of the skin image
            int rotationAmount = mSkinPixmapIsPortrait ? 0 : 90;
            // Adjust for user-initiated rotation
            switch (mOrientation) {
                case SKIN_ROTATION_0:    rotationAmount +=   0;   break;
                case SKIN_ROTATION_90:   rotationAmount +=  90;   break;
                case SKIN_ROTATION_180:  rotationAmount += 180;   break;
                case SKIN_ROTATION_270:  rotationAmount += 270;   break;
                default:                 rotationAmount +=   0;   break;
            }
            if (rotationAmount >= 360) rotationAmount -= 360;
            rotater.rotate(rotationAmount);
            QPixmap rotatedPMap(mRawSkinPixmap->transformed(rotater));

            // Scale the bitmap to the current window size
            int width = mContainer.width();
            QPixmap scaledPixmap = rotatedPMap.scaledToWidth(width);

            // Convert from bit map to a mask
            QBitmap bitmap = scaledPixmap.mask();
            setVisibleExtent(bitmap);

            // Determine if this is a round device.
            // If this mask is circular, it will be transparent near
            // the corners of the bounding rectangle. The top left of
            // the circle will be at (1 - 1/sqrt(2))*diameter = 0.146*w.
            QImage mapImage = bitmap.toImage();
            int height = mContainer.height();
            int opaqueWidth  = width  - mSkinGapLeft - mSkinGapRight;
            int opaqueHeight = height - mSkinGapTop  - mSkinGapBottom;
            int xTest = mSkinGapLeft + 0.146 * opaqueWidth  - 4;
            int yTest = mSkinGapTop  + 0.146 * opaqueHeight - 4;
            if (mapImage.size().width() > xTest) {
                // The map is not null. It is round if the test pixel is ones.
                // (If it is null, it's not doing anything, which is rectangular.)
                mBackingSurface->isRound = (mapImage.pixel(xTest, yTest) == 0xFFFFFFFF);
            }
#ifdef __APPLE__
            // On Mac, the mask is automatically stretched so its
            // rectangular extent is as big as the widget it is
            // applied to. To avoid stretching, set two points to
            // make the mask's extent the full widget size.
            QPainter painter(&bitmap);
            painter.setBrush(Qt::black);
            QPoint twoPoints[2] = { QPoint(bitmap.width()-1, 0),    // North east
                                    QPoint(0, bitmap.height()-1) }; // South west
            painter.drawPoints(twoPoints, 2);
#endif
            // Apply the mask
            mContainer.setMask(bitmap);
        }
    }
    mContainer.show();
    mToolWindow->dockMainWindow();

    if (haveFrame != mPreviouslyFramed) {
        // We are switching between framed and frameless. We need to
        // signal a kEventScreenChanged to force redrawing the device
        // screen.
        // Unfortunately, signaling this once isn't sufficient; we
        // need to do it until things settle down. Three time looks
        // like enough. Five times is for safety, but even this is
        // not a 100% fix.
        // TODO: http://b/78789992 Occasional black screen when switching framed/frameless
        mHardRefreshCountDown = 5;
        mPreviouslyFramed = haveFrame;
    }
    SkinEvent* event = new SkinEvent();
    if (mHardRefreshCountDown == 0) {
        D("%s: kEventWindowChanged", __FUNCTION__);
        event->type = kEventWindowChanged;
    } else {
        D("%s: kEventScreenChanged", __FUNCTION__);
        event->type = kEventScreenChanged;
        mHardRefreshCountDown--;
    }
    queueSkinEvent(event);
}

void EmulatorQtWindow::getSkinPixmap() {
    if (mRawSkinPixmap != nullptr) {
        // Already exists
        return;
    }
    mSkinPixmapIsPortrait = true; // Default assumption
    // We need to read the skin image.
    // Where is the skin?
    char *skinName;
    char *skinDir;
    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    // Parse the 'layout' file in the skin directory
    QString layoutPath = PathUtils::join(skinDir, skinName, "layout").c_str();
    AConfig* skinConfig = aconfig_node("", "");
    aconfig_load_file(skinConfig, layoutPath.toStdString().c_str());
    // Find the first instance of parts/*/background/image
    // ("*" will be either "portrait" or "landscape")
    AConfig* partsConfig = aconfig_find(skinConfig, "parts");
    if (partsConfig == nullptr) return; // Failed
    const char *skinFileName = nullptr;
    for (AConfig* partNode = partsConfig->first_child;
                  partNode != NULL;
                  partNode = partNode->next) {
        const AConfig* backgroundNode = aconfig_find(partNode, "background");
        if (backgroundNode == NULL) continue;
        skinFileName = aconfig_str(backgroundNode, "image", nullptr);
        if (skinFileName != nullptr && skinFileName[0] != '\0') {
            mSkinPixmapIsPortrait = !strcmp(partNode->name, "portrait");
            break;
        }
    }
    if (skinFileName == nullptr || skinFileName[0] == '\0') return; // Failed

    QString skinPath = QString(skinDir) + QDir::separator()
                       + skinName + QDir::separator()
                       + skinFileName;
    // Emulator UI hides the border of the skin image (frameless style), which
    // is done by mask off the transparent pixels of the skin image.
    // But from pixel4_a, the skin image also sets transparent pixels for the
    // display. To avoid masking off the display, we replace the alpha vaule of
    // these pixels as 255 (opage).
    QImage img(skinPath);
    for (int row = 0; row < img.height(); row++) {
        int left = -1, right = -1;
        for (int col = 0; col < img.width(); col++) {
            if (qAlpha(img.pixel(col, row)) != 0) {
                left = col;
                break;
            }
        }
        for (int col = img.width(); col >= 0; col--) {
            if (qAlpha(img.pixel(col, row)) != 0) {
                right = col;
                break;
            }
        }
        if (left == -1 || right == -1 || left == right) {
            continue;
        }
        for (int col = left; col <= right; col++) {
            QRgb pixel = img.pixel(col, row);
            if (qAlpha(pixel) == 0) {
                img.setPixel(col, row, qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 255));
            }
        }
    }
    mRawSkinPixmap = new QPixmap(QPixmap::fromImage(img));
}

void EmulatorQtWindow::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QRect bg(QPoint(0, 0), this->size());
    painter.fillRect(bg, Qt::black);

    if (mBackingSurface) {
        QRect r(0, 0, mBackingSurface->w, mBackingSurface->h);
        if (mBackingBitmapChanged) {
            mScaledBackingImage = QPixmap::fromImage(
                                       mBackingSurface->bitmap->get().scaled(
                                           r.size() * devicePixelRatioF(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
            mBackingBitmapChanged = false;
        }
        if (!mScaledBackingImage.isNull()) {
            if (mScaledBackingImage.devicePixelRatio() != devicePixelRatioF()) {
                mScaledBackingImage.setDevicePixelRatio(devicePixelRatioF());
            }
            painter.drawPixmap(r, mScaledBackingImage);
        }
    }
}

void EmulatorQtWindow::raise() {
    mContainer.raise();
    mToolWindow->raise();
    if (mCarClusterWindow) {
        mCarClusterWindow->raise();
    }
}

void EmulatorQtWindow::show() {
    if (mClosed) {
        return;
    }
    mContainer.show();
    QFrame::show();
    mToolWindow->show();
    if (mCarClusterWindow) {
        mCarClusterConnector->start();
    }

    QObject::connect(window()->windowHandle(), &QWindow::screenChanged, this,
                     &EmulatorQtWindow::onScreenChanged);
#if QT_VERSION >= 0x060000
    QObject::connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this,
                     &EmulatorQtWindow::onScreenConfigChanged);
    QObject::connect(qGuiApp, &QGuiApplication::screenAdded,
            this, &EmulatorQtWindow::onScreenConfigChanged);
    QObject::connect(qGuiApp, &QGuiApplication::screenRemoved,
            this, &EmulatorQtWindow::onScreenConfigChanged);
#else
    // On Mac, the above function won't be triggered when you plug in a new
    // monitor and the OS moves the emulator to the new screen. In such
    // situation, it will trigger screenCountChanged.
    QObject::connect(qApp->desktop(), &QDesktopWidget::screenCountChanged, this,
                     &EmulatorQtWindow::onScreenConfigChanged);
    QObject::connect(qApp->desktop(), &QDesktopWidget::primaryScreenChanged,
            this, &EmulatorQtWindow::onScreenConfigChanged);
    QObject::connect(qApp->desktop(), &QDesktopWidget::workAreaResized,
            this, &EmulatorQtWindow::onScreenConfigChanged);
#endif
}

void EmulatorQtWindow::setOnTop(bool onTop) {
    setFrameOnTop(&mContainer, onTop);
    setFrameOnTop(mToolWindow, onTop);
    if (mCarClusterWindow) {
        setFrameOnTop(mCarClusterWindow, onTop);
    }
}

void EmulatorQtWindow::setFrameAlways(bool frameAlways)
{
    mFrameAlways = frameAlways;
    if (mStartupDone) {
        maskWindowFrame();
        mContainer.show();
    }

    D("%s: kEventScreenChanged", __FUNCTION__);
    SkinEvent* event = new SkinEvent();
    event->type = kEventScreenChanged;
    queueSkinEvent(event);
}

void EmulatorQtWindow::setIgnoreWheelEvent(bool ignore) {
    mIgnoreWheelEvent = ignore;
}

void EmulatorQtWindow::setPauseAvdWhenMinimized(bool pause) {
    mPauseAvdWhenMinimized = pause;
}

void EmulatorQtWindow::showMinimized() {
#ifndef _WIN32
    // For Mac and Linux, ensure that the window has a frame and
    // minimize buttons while it is minimized.
    // Why?
    //   -  Mac won't un-minimize unless those exist.
    //   -  Linux adds those in automatically on minimize. This
    //      code path gets us to remove them on un-minimize.

    Qt::WindowFlags flags = mContainer.windowFlags();
#ifdef __linux__
    flags &= ~FRAME_WINDOW_FLAGS_MASK;
    flags |= FRAMED_WINDOW_FLAGS;
#else // __APPLE__
    if (hasFrame()) {
        flags |= Qt::NoDropShadowWindowHint;
    } else {
        // Ignore the previous flags
        flags = Qt::Window | Qt::WindowMinMaxButtonsHint;
    }
#endif // __APPLE__
    mContainer.setWindowFlags(flags);

#endif // !_WIN32
    mContainer.showMinimized();
    mWindowIsMinimized = true;

    if (mPauseAvdWhenMinimized) {
        (*mAdbInterface)->runAdbCommand( {"emu", "avd", "pause"},
                [this](const android::emulation::OptionalAdbCommandResult&) { ; }, 5000);
    }
}

void EmulatorQtWindow::handleTouchPoints(const QTouchEvent& touch, uint32_t displayId) {
    unsigned char nrOfPoints = 0;
    unsigned char lastValidTouchPointIndex = 0;
    for(int i=touch.touchPoints().count()-1;i>=0;--i)
    {
        if( touch.touchPoints()[i].state()!= Qt::TouchPointStationary )
        {
            lastValidTouchPointIndex = i;
            break;
        }
    }
    for (auto const& elem : touch.touchPoints()) {
        SkinEvent* skin_event = nullptr;
        bool eventSet = false;
        if (elem.state() == Qt::TouchPointPressed) {
            skin_event = createSkinEvent(kEventTouchBegin);
            skin_event->type = kEventTouchBegin;
            eventSet = true;
        } else if (elem.state() == Qt::TouchPointMoved) {
            skin_event = createSkinEvent(kEventTouchUpdate);
            skin_event->type = kEventTouchUpdate;
            eventSet = true;
        } else if (elem.state() == Qt::TouchPointReleased) {
            skin_event = createSkinEvent(kEventTouchEnd);
            skin_event->type = kEventTouchEnd;
            eventSet = true;
        }
        if (eventSet) {
            bool lasElementFound = false;
            skin_event->u.multi_touch_point.x = elem.pos().x();
            skin_event->u.multi_touch_point.y = elem.pos().y();
            skin_event->u.multi_touch_point.x_global = elem.screenPos().x();
            skin_event->u.multi_touch_point.y_global = elem.screenPos().y();
            skin_event->u.multi_touch_point.id = elem.id();
            skin_event->u.multi_touch_point.pressure =
                    (int)(elem.pressure() * MTS_PRESSURE_RANGE_MAX);
            skin_event->u.multi_touch_point.touch_minor =
                    std::min(elem.ellipseDiameters().rheight(),
                             elem.ellipseDiameters().rwidth());
            skin_event->u.multi_touch_point.touch_major =
                    std::max(elem.ellipseDiameters().rheight(),
                             elem.ellipseDiameters().rwidth());
            skin_event->u.multi_touch_point.orientation=elem.rotation();
            if (nrOfPoints == lastValidTouchPointIndex)
            {
                skin_event->u.multi_touch_point.skip_sync=false;
                lasElementFound = true;
            }
            else
            {
                skin_event->u.multi_touch_point.skip_sync=true;
            }
            skin_event->u.multi_touch_point.display_id = displayId;
            queueSkinEvent(skin_event);
            if(lasElementFound)
            {
                break;
            }
        }
        ++nrOfPoints;
    }
}

bool EmulatorQtWindow::event(QEvent* ev) {
    if (ev->type() == QEvent::TouchBegin || ev->type() == QEvent::TouchUpdate ||
        ev->type() == QEvent::TouchEnd) {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(ev);
        handleTouchPoints(*touchEvent);
        return true;
    }
    if (ev->type() == QEvent::WindowActivate && mWindowIsMinimized) {
        mWindowIsMinimized = false;
        // note: this is intentional, as user could pause the avd throu console
        // and we want to make the avd usable.
        if(true) {
            (*mAdbInterface)->runAdbCommand( {"emu", "avd", "resume"},
                    [this](const android::emulation::OptionalAdbCommandResult&) { ; }, 5000);
            (*mAdbInterface)->runAdbCommand( {"shell", "input", "keyevent", "KEYCODE_WAKEUP"},
                    [this](const android::emulation::OptionalAdbCommandResult&) { ; }, 5000);
            TelephonyPage::updateModemTime();
        }
#ifndef _WIN32
        // When we minimized, we re-enabled the window frame (because Mac won't un-minimize
        // a frameless window). Disable the window frame now, if it should be disabled.
        Qt::WindowFlags flags = mContainer.windowFlags();
#ifdef __linux__
        flags &= ~FRAME_WINDOW_FLAGS_MASK;
        flags |= (hasFrame() ? FRAMED_WINDOW_FLAGS : FRAMELESS_WINDOW_FLAGS);
        flags &= ~Qt::WindowMinimizeButtonHint;
#else // __APPLE__
        if (hasFrame()) {
            flags &= ~Qt::NoDropShadowWindowHint;
        } else {
            flags &= ~FRAME_WINDOW_FLAGS_MASK;
            flags |= FRAMELESS_WINDOW_FLAGS;
        }
#endif
        mContainer.setWindowFlags(flags);
        show();

        // Trigger a ScreenChanged event so the device
        // screen will refresh immediately
        SkinEvent* event = new SkinEvent();
        event->type = kEventScreenChanged;
        queueSkinEvent(event);
#endif // !_WIN32
    }

    return QWidget::event(ev);
}

void EmulatorQtWindow::startThread(StartFunction f, int argc, char** argv) {
    if (!mMainLoopThread) {
        // Check for null as arguments to StartFunction
        if (argc && !argv) {
            D("Empty argv passed to a startThread(), returning early");
            return;
        }

        // pass the QEMU main thread's arguments into the crash handler
        std::string arguments = "===== QEMU main loop arguments =====\n";
        for (int i = 0; i < argc; ++i) {
            // Check for null in argv
            if (!argv[i]) {
                D("Internal Error: argv[%d] was null, replaced with empty "
                  "string",
                  i);
                argv[i] = strdup("");
            }
            arguments += argv[i];
            arguments += '\n';
        }
        CrashReporter::get()->attachData(
                "qemu-main-loop-args.txt", arguments);

        mMainLoopThread = new MainLoopThread(f, argc, argv);
        QObject::connect(mMainLoopThread, &QThread::finished, &mContainer,
                         &EmulatorContainer::close);
        mMainLoopThread->start();
    } else {
        D("mMainLoopThread already started");
    }
}

void EmulatorQtWindow::slot_clearInstance() {
#ifndef __APPLE__
    if (mToolWindow) {
        delete mToolWindow;
        mToolWindow = NULL;
    }
    if (mCarClusterWindow) {
        delete mCarClusterWindow;
        mCarClusterWindow = NULL;
    }
#endif

    skin_winsys_save_window_pos();
    // Force kill any parallel tasks that may be running, as this can make Qt hang on exit.
    System::get()->cleanupWaitingPids();
    sInstance.get().reset();
}

void EmulatorQtWindow::slot_blit(SkinSurfaceBitmap* src,
                                 QRect srcRect,
                                 SkinSurfaceBitmap* dst,
                                 QPoint dstPos,
                                 QPainter::CompositionMode op,
                                 QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    if (mBackingSurface && dst == mBackingSurface->bitmap) {
        mBackingBitmapChanged = true;
    }
    dst->drawFrom(src, dstPos, srcRect, op);
}

void EmulatorQtWindow::slot_fill(SkinSurface* s,
                                 QRect rect,
                                 QColor color,
                                 QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    if (mBackingSurface && s == mBackingSurface) {
        mBackingBitmapChanged = true;
    }
    s->bitmap->fill(rect, color);
}

void EmulatorQtWindow::slot_getDevicePixelRatio(double* out_dpr,
                                                QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    *out_dpr = devicePixelRatioF();
}

void EmulatorQtWindow::slot_getScreenDimensions(QRect* out_rect,
                                                QSemaphore* semaphore) {
    D("slot_getScreenDimensions: begin");
    QSemaphoreReleaser semReleaser(semaphore);
    out_rect->setRect(0, 0, 1920, 1080); // Arbitrary default

    D("slot_getScreenDimensions: Getting screen geometry");
#if QT_VERSION >= 0x060000
    auto newScreen = window()->windowHandle() ? window()->windowHandle()->screen() : nullptr;
    if (!newScreen) {
        D("Can't get screen geometry. Window is off screen.");
    }
    QRect rect = newScreen->geometry();
#else
    QRect rect = ((QApplication*)QApplication::instance())
                         ->desktop()
                         ->screenGeometry();
#endif
    D("slot_getScreenDimensions: Getting screen geometry (done)");
    out_rect->setX(rect.x());
    out_rect->setY(rect.y());

    // Always report slightly smaller-than-actual dimensions to prevent odd
    // resizing behavior, which can happen if things like the OSX dock are
    // not taken into account. The difference below is specifically to take
    // into account the OSX dock.
    out_rect->setWidth(rect.width() * .95);
#ifdef __APPLE__
    out_rect->setHeight(rect.height() * .85);
#else  // _WIN32 || __linux__
    out_rect->setHeight(rect.height() * .95);
#endif
    D("slot_getScreenDimensions: end");
}

WId EmulatorQtWindow::getWindowId() {
    WId wid = effectiveWinId();
    D("Effective win ID is %lx", wid);
#if defined(__APPLE__)
    wid = (WId)getNSWindow((void*)wid);
    D("After finding parent, win ID is %lx", wid);
#endif
    return wid;
}

void EmulatorQtWindow::slot_getWindowSize(int* ww,
                                          int* hh,
                                          QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    QRect geom = mContainer.geometry();

    *ww = geom.width();
    *hh = geom.height();
}

void EmulatorQtWindow::slot_getWindowPos(int* xx,
                                         int* yy,
                                         QSemaphore* semaphore) {
    // Note that mContainer.x() == mContainer.frameGeometry().x(), which
    // is NOT what we want.

    QSemaphoreReleaser semReleaser(semaphore);
    QRect geom = mContainer.geometry();

    *xx = geom.x();
    *yy = geom.y();
}

void EmulatorQtWindow::slot_windowHasFrame(bool* outValue, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    *outValue = hasFrame();
}

void EmulatorQtWindow::slot_getFramePos(int* xx,
                                        int* yy,
                                        QSemaphore* semaphore) {
    // Note that mContainer.x() == mContainer.frameGeometry().x(), which
    // is what we want.

    QSemaphoreReleaser semReleaser(semaphore);
    *xx = mContainer.x();
    *yy = mContainer.y();
}

void EmulatorQtWindow::slot_isWindowFullyVisible(bool* out_value,
                                                 QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
#if QT_VERSION >= 0x060000
    auto newScreen = mContainer.window()->windowHandle() ?
            mContainer.window()->windowHandle()->screen() : nullptr;
    if (!newScreen) {
        // Window is not on any screen
        *out_value = false;
    } else {
        QRect screenGeo = newScreen->geometry();
        *out_value = screenGeo.contains(mContainer.geometry());
    }
#else
    QDesktopWidget* desktop =
            ((QApplication*)QApplication::instance())->desktop();
    int screenNum =
            desktop->screenNumber(&mContainer);  // Screen holding the app

    if (screenNum < 0) {
        // Window is not on any screen
        *out_value = false;
    } else {
        QRect screenGeo = desktop->screenGeometry(screenNum);
        *out_value = screenGeo.contains(mContainer.geometry());
    }
#endif
}

void EmulatorQtWindow::slot_isWindowOffScreen(bool* out_value,
                                              QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
#if QT_VERSION >= 0x060000
    auto newScreen = mContainer.window()->windowHandle() ?
            mContainer.window()->windowHandle()->screen() : nullptr;
    *out_value = (newScreen == nullptr);
#else
    QDesktopWidget* desktop =
            ((QApplication*)QApplication::instance())->desktop();
    int screenNum =
            desktop->screenNumber(&mContainer);  // Screen holding the app

    *out_value = (screenNum < 0);
#endif  // QT_VERSION
}

void EmulatorQtWindow::pollEvent(SkinEvent* event,
                                 bool* hasEvent) {
    android::base::AutoLock lock(mSkinEventQueueLock);
    if (mSkinEventQueue.isEmpty()) {
        lock.unlock();
        *hasEvent = false;
    } else {
        SkinEvent* newEvent = mSkinEventQueue.dequeue();
        lock.unlock();
        *hasEvent = true;

        memcpy(event, newEvent, sizeof(SkinEvent));
        delete newEvent;
    }
}

void EmulatorQtWindow::queueSkinEvent(SkinEvent* event) {
    const auto rotationEventLayout =
            event->type == kEventLayoutRotate
                ? makeOptional(event->u.layout_rotation.rotation)
                : kNullopt;

    android::base::AutoLock lock(mSkinEventQueueLock);
    const bool firstEvent = mSkinEventQueue.isEmpty();

    // For the following events, only the "last" example of said event
    // matters, so ensure that there is only one of them in the queue at a
    // time. Additionaly the screen changed event processing is very slow,
    // so let's not generate too many of them.
    bool replaced = false;
    const auto type = event->type;
    if (type == kEventScrollBarChanged ||
        type == kEventZoomedWindowResized ||
        type == kEventScreenChanged) {
        for (int i = 0; i < mSkinEventQueue.size(); i++) {
            if (mSkinEventQueue.at(i)->type == type) {
                SkinEvent* toDelete = mSkinEventQueue.at(i);
                mSkinEventQueue.replace(i, event);
                lock.unlock();
                delete toDelete;
                replaced = true;
                break;
            }
        }
    }

    if (!replaced) {
        mSkinEventQueue.enqueue(event);
        lock.unlock();
    }

    const auto uiAgent = mToolWindow->getUiEmuAgent();
    if (firstEvent && uiAgent && uiAgent->userEvents &&
        uiAgent->userEvents->onNewUserEvent) {
        // We know that as soon as emulator starts processing user events
        // it processes them until there are none. So we should only notify it
        // if this event is the first one.
        uiAgent->userEvents->onNewUserEvent();
    }
    if (rotationEventLayout) {
        emit(layoutChanged(*rotationEventLayout));
    }
}

void EmulatorQtWindow::slot_updateRotation(SkinRotation rotation) {
    mOrientation = rotation;
    emit(layoutChanged(rotation));

    fixScale();
}

void EmulatorQtWindow::slot_releaseBitmap(SkinSurface* s,
                                          QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    if (mBackingSurface == s) {
        mBackingSurface = NULL;
        mBackingBitmapChanged = true;
    }
    delete s->bitmap;
    delete s;
}

void EmulatorQtWindow::slot_requestClose(QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    crashhandler_exitmode(__FUNCTION__);
    mContainer.close();
}

void EmulatorQtWindow::slot_requestUpdate(QRect rect,
                                          QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    if (!mBackingSurface)
        return;
    if (!mBackingSurface->bitmap)
        return;
    if (!mBackingSurface->bitmap->size().width() ||
        !mBackingSurface->bitmap->size().height())
        return;

    QRect r(rect.x() * mBackingSurface->w / mBackingSurface->bitmap->size().width(),
            rect.y() * mBackingSurface->h / mBackingSurface->bitmap->size().height(),
            rect.width() * mBackingSurface->w / mBackingSurface->bitmap->size().width(),
            rect.height() * mBackingSurface->h / mBackingSurface->bitmap->size().height());
    update(r);
}

void EmulatorQtWindow::slot_setDeviceGeometry(QRect rect,
                                              QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mDeviceGeometry = rect;
}

void EmulatorQtWindow::slot_setWindowPos(int x, int y, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mContainer.move(x, y);
}

void EmulatorQtWindow::slot_setWindowSize(int w, int h, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mContainer.resize(w, h);
}

void EmulatorQtWindow::slot_paintWindowOverlayForResize(int mouseX, int mouseY, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mOverlay.paintForResize(mouseX, mouseY);
}

void EmulatorQtWindow::slot_clearWindowOverlay(QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mOverlay.hide();
}

void EmulatorQtWindow::slot_setWindowOverlayForResize(int whichCorner, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mOverlay.showForResize(whichCorner);
}

void EmulatorQtWindow::slot_setWindowCursorResize(int whichCorner, QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mContainer.setCursor((whichCorner == 0 || whichCorner == 2) ?
                              Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
}

void EmulatorQtWindow::slot_setWindowCursorNormal(QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mContainer.setCursor(Qt::ArrowCursor);
}

void EmulatorQtWindow::slot_setWindowIcon(const unsigned char* data,
                                          int size,
                                          QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    QPixmap image;
    image.loadFromData(data, size);
    QIcon icon(image);
    QApplication::setWindowIcon(icon);
}

void EmulatorQtWindow::slot_setWindowTitle(QString title,
                                           QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    mContainer.setWindowTitle(title);

    // This is the first time that we know the android_serial_number_port
    // has been set. This port ensures AdbInterface can identify the correct
    // device if there is more than one.
    (*mAdbInterface)->setSerialNumberPort(android_serial_number_port);
}

void EmulatorQtWindow::slot_showWindow(SkinSurface* surface,
                                       QRect rect,
                                       QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    if (mClosed) {
        return;
    }
    if (android_cmdLineOptions->qt_hide_window) {
        return;
    }
    if (surface != mBackingSurface) {
        mBackingBitmapChanged = true;
        mBackingSurface = surface;
    }

    showNormal();
    setFixedSize(rect.size());

    if (rect.width() < mContainer.minimumWidth() ||
        rect.height() < mContainer.minimumHeight())
    {
        // We're trying to go smaller than the container will allow.
        // Increase our size to the container's minimum.
        double horizIncreaseFactor = (double)(mContainer.minimumWidth()) / rect.width();
        double vertIncreaseFactor = (double)(mContainer.minimumHeight()) / rect.height();
        if (horizIncreaseFactor > vertIncreaseFactor) {
            rect.setWidth(mContainer.minimumWidth());
            rect.setHeight((int)(horizIncreaseFactor * rect.height()));
        } else {
            rect.setWidth((int)(vertIncreaseFactor * rect.width()));
            rect.setHeight(mContainer.minimumHeight());
        }
    }

    // If this was the result of a zoom, don't change the overall window size,
    // and adjust the scroll bars to reflect the desired focus point.
    if (mInZoomMode && mNextIsZoom) {
        mContainer.stopResizeTimer();
        recenterFocusPoint();
    } else if (!mNextIsZoom) {
        mContainer.resize(rect.size());
    }
    mNextIsZoom = false;

    maskWindowFrame();
    show();

    // Zooming forces the scroll bar to be visible for sizing purposes. They
    // should never be shown when not in zoom mode, and should only show when
    // necessary when in zoom mode.
    if (mInZoomMode) {
        mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    // If the user isn't using an x86 AVD, make sure it's because their machine
    // doesn't support CPU acceleration. If it does, recommend switching to an
    // x86 AVD. This cannot be done on the construction of the window since the
    // Qt UI thread has not been properly initialized yet.
    if (mFirstShowWindowCall) {
        showWin32DeprecationWarning();
        showAvdArchWarning();
        checkShouldShowGpuWarning();
        showGpuWarning();
        checkAdbVersionAndWarn();
#ifdef _WIN32
        checkVgkAndWarn();
#endif
        checkNestedAndWarn();
        mFirstShowWindowCall = false;
    }
}

void EmulatorQtWindow::onScreenChanged(QScreen* newScreen) {
    if (newScreen != mCurrentScreen) {
        D("%s: kEventScreenChanged", __FUNCTION__);
        queueSkinEvent(createSkinEvent(kEventScreenChanged));
        mCurrentScreen = newScreen;
    }
}

void EmulatorQtWindow::onScreenConfigChanged() {
    auto newScreen = window()->windowHandle() ?
            window()->windowHandle()->screen() : nullptr;
    if (newScreen != mCurrentScreen) {
        D("%s: kEventScreenChanged", __FUNCTION__);
        queueSkinEvent(createSkinEvent(kEventScreenChanged));
        mCurrentScreen = newScreen;
    }
}

void EmulatorQtWindow::showEvent(QShowEvent* event) {
    mStartupTimer.stop();
    mStartupDialog->hide();
    mStartupDialog.clear();
    mStartupDone = true;

    if (mClosed) {
        event->ignore();
        return;
    }
    if (mFirstShowEvent) {
        // moved from android_metrics_start() in metrics.cpp
        android_metrics_start_adb_liveness_checker((*mAdbInterface));
        if (android_hw->test_quitAfterBootTimeOut > 0) {
            android_test_start_boot_complete_timer(android_hw->test_quitAfterBootTimeOut);
        }
        mFirstShowEvent = false;
    }
}

void EmulatorQtWindow::slot_horizontalScrollChanged(int value) {
    simulateScrollBarChanged(value, mContainer.verticalScrollBar()->value());
}

void EmulatorQtWindow::slot_verticalScrollChanged(int value) {
    simulateScrollBarChanged(mContainer.horizontalScrollBar()->value(), value);
}

void EmulatorQtWindow::slot_scrollRangeChanged(int min, int max) {
    simulateScrollBarChanged(mContainer.horizontalScrollBar()->value(),
                             mContainer.verticalScrollBar()->value());
}

void EmulatorQtWindow::screenshot() {
    QString savePath = getScreenshotSaveDirectory();
    if (savePath.isEmpty()) {
        showErrorDialog(tr("The screenshot save location is not set.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Screenshot"));
        return;
    }

    if (!android::emulation::captureScreenshot(savePath.toStdString())) {
        showErrorDialog(tr("Screenshot failed"), tr("Screenshot"));
    } else {
        // Display the flash animation immediately as feedback - if it fails, an
        // error dialog will indicate as such.
        mOverlay.showAsFlash();
    }
}

void EmulatorQtWindow::slot_installCanceled() {
    if (mApkInstallCommand && mApkInstallCommand->inFlight()) {
        mApkInstallCommand->cancel();
    }
}

void EmulatorQtWindow::runAdbInstall(const QString& path) {
    if (mApkInstallCommand && mApkInstallCommand->inFlight()) {
        // Modal dialogs should prevent this.
        return;
    }

    // Show a dialog so the user knows something is happening
    mInstallDialog->show();

    mApkInstallCommand = mApkInstaller->install(
            path.toStdString(),
            [this](ApkInstaller::Result result, std::string errorString) {
                runOnUiThread([this, result, errorString] {
                    installDone(result, errorString);
                });
            });
}

void EmulatorQtWindow::installDone(ApkInstaller::Result result,
                                   StringView errorString) {
    mInstallDialog->hide();

    QString msg;
    switch (result) {
        case ApkInstaller::Result::kSuccess:
            return;

        case ApkInstaller::Result::kOperationInProgress:
            msg =
                    tr("Another APK install is already in progress.<br/>"
                       "Please try again after it completes.");
            break;

        case ApkInstaller::Result::kApkPermissionsError:
            msg =
                    tr("Unable to read the given APK.<br/>"
                       "Ensure that the file is readable.");
            break;

        case ApkInstaller::Result::kAdbConnectionFailed:
            msg =
                    tr("Failed to start adb.<br/>"
                       "Check settings to verify your chosen adb path is "
                       "valid.");
            break;

        case ApkInstaller::Result::kInstallFailed:
            msg = tr("The APK failed to install.<br/> Error: %1")
                          .arg(c_str(errorString).get());
            break;

        default:
            msg = tr("There was an unknown error while installing the APK.");
    }

    showErrorDialog(msg, tr("APK Installer"));
}

void EmulatorQtWindow::runAdbPush(const QList<QUrl>& urls) {
    std::vector<std::pair<std::string, std::string>> file_paths;
    StringView remoteDownloadsDir = avdInfo_getApiLevel(android_avdInfo) > 10
                                            ? kRemoteDownloadsDir
                                            : kRemoteDownloadsDirApi10;
    for (const auto& url : urls) {
        string remoteFile = PathUtils::join(remoteDownloadsDir,
                                            url.fileName().toStdString());
        file_paths.push_back(
                std::make_pair(url.toLocalFile().toStdString(), remoteFile));
    }

    mFilePusher->pushFiles(file_paths);
}

void EmulatorQtWindow::slot_adbPushCanceled() {
    mFilePusher->cancel();
}

void EmulatorQtWindow::slot_showMessage(QString text,
                                        Ui::OverlayMessageType messageType,
                                        int timeoutMs) {
    if (!android_cmdLineOptions->qt_hide_window)
        mContainer.messageCenter().addMessage(text, messageType, timeoutMs);
}

void EmulatorQtWindow::slot_showMessageWithDismissCallback(QString text,
                                                           Ui::OverlayMessageType messageType,
                                                           QString dismissText,
                                                           RunOnUiThreadFunc func,
                                                           int timeoutMs) {
    if (!android_cmdLineOptions->qt_hide_window) {
        auto msg = mContainer.messageCenter().addMessage(text, messageType,
                                                         timeoutMs);
        msg->setDismissCallback(dismissText, std::move(func));
    }
}

void EmulatorQtWindow::adbPushProgress(double progress, bool done) {
    if (done) {
        mPushDialog->hide();
        return;
    }

    mPushDialog->setValue(progress * kPushProgressBarMax);
    mPushDialog->show();
}

void EmulatorQtWindow::adbPushDone(StringView filePath,
                                   FilePusher::Result result) {
    QString msg;
    switch (result) {
        case FilePusher::Result::Success:
            return;
        case FilePusher::Result::ProcessStartFailure:
            msg = tr("Could not launch process to copy %1")
                          .arg(c_str(filePath).get());
            break;
        case FilePusher::Result::FileReadError:
            msg = tr("Could not locate %1").arg(c_str(filePath).get());
            break;
        case FilePusher::Result::AdbPushFailure:
            msg = tr("'adb push' failed for %1").arg(c_str(filePath).get());
            break;
        default:
            msg = tr("Could not copy %1").arg(c_str(filePath).get());
    }
    showErrorDialog(msg, tr("File Copy"));
}

// Convert a Qt::Key_XXX code into the corresponding Linux keycode value.
// On failure, return -1.
static int convertKeyCode(int sym) {
#define KK(x, y) \
    { Qt::Key_##x, LINUX_KEY_##y }
#define K1(x) KK(x, x)
    static const struct {
        int qt_sym;
        int keycode;
    } kConvert[] = {
            KK(Left, LEFT),
            KK(Right, RIGHT),
            KK(Up, UP),
            KK(Down, DOWN),
            K1(0),
            K1(1),
            K1(2),
            K1(3),
            K1(4),
            K1(5),
            K1(6),
            K1(7),
            K1(8),
            K1(9),
            K1(F1),
            K1(F2),
            K1(F3),
            K1(F4),
            K1(F5),
            K1(F6),
            K1(F7),
            K1(F8),
            K1(F9),
            K1(F10),
            K1(F11),
            K1(F12),
            K1(A),
            K1(B),
            K1(C),
            K1(D),
            K1(E),
            K1(F),
            K1(G),
            K1(H),
            K1(I),
            K1(J),
            K1(K),
            K1(L),
            K1(M),
            K1(N),
            K1(O),
            K1(P),
            K1(Q),
            K1(R),
            K1(S),
            K1(T),
            K1(U),
            K1(V),
            K1(W),
            K1(X),
            K1(Y),
            K1(Z),
            KK(Minus, MINUS),
            KK(Equal, EQUAL),
            KK(Backspace, BACKSPACE),
            KK(Home, HOME),
            KK(Escape, ESC),
            KK(Comma, COMMA),
            KK(Period, DOT),
            KK(Space, SPACE),
            KK(Slash, SLASH),
            KK(Return, ENTER),
            KK(Tab, TAB),
            KK(BracketLeft, LEFTBRACE),
            KK(BracketRight, RIGHTBRACE),
            KK(Backslash, BACKSLASH),
            KK(Semicolon, SEMICOLON),
            KK(Apostrophe, APOSTROPHE),
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (sym == kConvert[nn].qt_sym) {
            return kConvert[nn].keycode;
        }
    }
    if (!android_hw->hw_arc) return -1;
    static const struct {
        int qt_sym;
        int keycode;
    } kCrosConvert[] = {
            // Qt treats "SHIFT + TAB" as "Backtab", just convert it back to TAB.
            KK(Backtab, TAB),
            KK(Control, LEFTCTRL),
            KK(Alt, LEFTALT),
            KK(Shift, LEFTSHIFT),
    };
    const size_t kCrosConvertSize = sizeof(kCrosConvert) / sizeof(kCrosConvert[0]);
    for (nn = 0; nn < kCrosConvertSize; ++nn) {
        if (sym == kCrosConvert[nn].qt_sym) {
            return kCrosConvert[nn].keycode;
        }
    }
    return -1;
}

SkinEvent* EmulatorQtWindow::createSkinEvent(SkinEventType type) {
    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = type;
    return skin_event;
}

void EmulatorQtWindow::doResize(const QSize& size,
                                bool isKbdShortcut) {
    if (mClosed) {
        return;
    }
    if (!mBackingSurface) {
        return;
    }
    int originalWidth = mBackingSurface->bitmap->size().width();
    int originalHeight = mBackingSurface->bitmap->size().height();

    auto newSize = QSize(originalWidth,
                         originalHeight).scaled(size, Qt::KeepAspectRatio);

    // Make sure the new size is always a little bit smaller than the
    // screen to prevent keyboard shortcut scaling from making a window
    // too large for the screen, which can result in the showing of the
    // scroll bars. This is not an issue when resizing by dragging the
    // corner because the OS will prevent too large a window.
    if (isKbdShortcut) {
        QRect screenDimensions;
        slot_getScreenDimensions(&screenDimensions);

        if (newSize.width() > screenDimensions.width() ||
            newSize.height() > screenDimensions.height()) {
            newSize.scale(screenDimensions.size(), Qt::KeepAspectRatio);
        }
    }

    double widthScale = (double)newSize.width() / (double)originalWidth;
    double heightScale = (double)newSize.height() / (double)originalHeight;

    // On HiDPI screen, newSize is in Qt logical pixel, whle originalWidth/
    // originalHeight are in actual pixel (logical pixel * pixel ratio).
    // When HiDPI scaling is disabled, we need to multiply the pixel ratio to
    // widthScale/heightScale to make the scale correct.
    if (android_cmdLineOptions->no_hidpi_scaling) {
        double dpr = 1.0;
#ifdef __APPLE__
        slot_getDevicePixelRatio(&dpr);
#endif
        widthScale *= dpr;
        heightScale *= dpr;
    }

    simulateSetScale(std::max(.2, std::min(widthScale, heightScale)));

    maskWindowFrame();
#ifdef __APPLE__
    // To fix issues when resizing + linking against macos sdk 11.
    SkinEvent* changeEvent = new SkinEvent();
    changeEvent->type = kEventScreenChanged;
    queueSkinEvent(changeEvent);
#endif
}

void EmulatorQtWindow::resizeAndChangeAspectRatio(bool isFolded) {
    QRect windowGeo = this->geometry();
    if (!mBackingSurface) {
        VLOG(foldable) << "backing surface not ready, cancel window adjustment";
        return;
    }
    QSize backingSize = mBackingSurface->bitmap->size();
    float scale = (float)windowGeo.width() / (float)backingSize.width();
    int displayX = android_hw->hw_lcd_width;
    int displayY = android_hw->hw_lcd_height;

    if (isFolded) {
        int displayXFolded;
        int displayYFolded;
        android_foldable_get_folded_area(nullptr, nullptr, &displayXFolded, &displayYFolded);
            switch(mOrientation) {
                default:
                case SKIN_ROTATION_180:
                case SKIN_ROTATION_0:
                    if (backingSize.width() == displayXFolded &&
                        backingSize.height() == displayYFolded) {
                        break;
                    }
                    windowGeo.setWidth((int)(displayXFolded * scale));
                    windowGeo.setHeight((int)(displayYFolded * scale));
                    backingSize.setWidth(displayXFolded);
                    backingSize.setHeight(displayYFolded);
                    break;
                case SKIN_ROTATION_90:
                case SKIN_ROTATION_270:
                    if (backingSize.width() == displayYFolded &&
                        backingSize.height() == displayXFolded) {
                        break;
                    }
                    backingSize.setWidth(displayYFolded);
                    backingSize.setHeight(displayXFolded);
                    windowGeo.setWidth((int)(displayYFolded * scale));
                    windowGeo.setHeight((int)(displayXFolded * scale));
                    break;
            }
    } else {
        switch(mOrientation) {
            default:
            case SKIN_ROTATION_0:
            case SKIN_ROTATION_180:
                if (backingSize.width() == displayX &&
                    backingSize.height() == displayY) {
                    break;
                }
                windowGeo.setWidth((int)(displayX * scale));
                windowGeo.setHeight((int)(displayY * scale));
                backingSize.setWidth(displayX);
                backingSize.setHeight(displayY);
                break;
            case SKIN_ROTATION_90:
            case SKIN_ROTATION_270:
                if (backingSize.width() == displayY &&
                    backingSize.height() == displayX) {
                    break;
                }
                windowGeo.setWidth((int)(displayY* scale));
                windowGeo.setHeight((int)(displayX* scale));
                backingSize.setWidth(displayY);
                backingSize.setHeight(displayX);
                break;
        }
    }
    setDisplayRegion(0, 0, backingSize.width(), backingSize.height());
    simulateSetScale(std::max(.2, (double)scale));
    QRect containerGeo = mContainer.geometry();
    mContainer.setGeometry(containerGeo.x(), containerGeo.y(), windowGeo.width(), windowGeo.height());
}

void EmulatorQtWindow::resizeAndChangeAspectRatio(int x, int y, int w, int h,
                                                  bool ignoreOrientation) {
    if (!mBackingSurface) {
        return;
    }
    QRect windowGeo = this->geometry();
    QSize backingSize = mBackingSurface->bitmap->size();
    float scale = (float)windowGeo.width() / (float)backingSize.width();
    if (!ignoreOrientation) {
        switch(mOrientation) {
            case SKIN_ROTATION_90:
            case SKIN_ROTATION_270:
                std::swap(w, h);
                std::swap(x, y);
                break;
            case SKIN_ROTATION_0:
            case SKIN_ROTATION_180:
            default:
                break;
        }
    }
    windowGeo.setWidth((int)(w * scale));
    windowGeo.setHeight((int)(h * scale));
    backingSize.setWidth(w);
    backingSize.setHeight(h);
    setDisplayRegionAndUpdate(x, y, backingSize.width(), backingSize.height());
    simulateSetScale(std::max(.2, (double)scale));
    QRect containerGeo = mContainer.geometry();
    mContainer.setGeometry(containerGeo.x(), containerGeo.y(), windowGeo.width(), windowGeo.height());
}

SkinMouseButtonType EmulatorQtWindow::getSkinMouseButton(
                                    const QMouseEvent* event) const {
    switch (event->button()) {
    case Qt::LeftButton:
        return kMouseButtonLeft;
    case Qt::RightButton:
        return kMouseButtonRight;
    case Qt::MiddleButton:
        return kMouseButtonMiddle;
    default:
        return kMouseButtonLeft;
    }
}

#if QT_VERSION >= 0x060000
void EmulatorQtWindow::handleMouseEvent(SkinEventType type,
                                        SkinMouseButtonType button,
                                        const QPointF& posF,
                                        const QPointF& gPosF,
                                        bool skipSync) {
    QPoint pos((int) posF.x(), (int) posF.y());
    QPoint gPos((int) gPosF.x(), (int) gPosF.y());
#else
void EmulatorQtWindow::handleMouseEvent(SkinEventType type,
                                        SkinMouseButtonType button,
                                        const QPoint& pos,
                                        const QPoint& gPos,
                                        bool skipSync) {
#endif  // QT_VERSION
    if (type == kEventMouseButtonDown) {
        mToolWindow->reportMouseButtonDown();
    }

    SkinEvent* skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = button;
    skin_event->u.mouse.skip_sync = skipSync;
    skin_event->u.mouse.x = pos.x();
    skin_event->u.mouse.y = pos.y();
    skin_event->u.mouse.x_global = gPos.x();
    skin_event->u.mouse.y_global = gPos.y();

    skin_event->u.mouse.xrel = pos.x() - mPrevMousePosition.x();
    skin_event->u.mouse.yrel = pos.y() - mPrevMousePosition.y();
    skin_event->u.mouse.display_id = 0;
    mPrevMousePosition = pos;

    queueSkinEvent(skin_event);
}

// Stores all the information of a pen event and
// adds the skin event to the queue
void EmulatorQtWindow::handlePenEvent(SkinEventType type,
                                      const QTabletEvent* event,
                                      bool skipSync) {
    SkinEvent* skin_event = createSkinEvent(type);
    skin_event->u.pen.tracking_id = event->uniqueId();
    skin_event->u.pen.pressure =
                        (int)(event->pressure() * MTS_PRESSURE_RANGE_MAX);
    skin_event->u.pen.orientation =
                penOrientation(tiltToRotation(event->xTilt(), event->yTilt()));
    skin_event->u.pen.button_pressed =
                    ((event->buttons() & Qt::RightButton) == Qt::RightButton);
#if QT_VERSION >= 0x060000
    skin_event->u.pen.rubber_pointer =
                            (event->pointerType() == QPointingDevice::PointerType::Eraser);
#else
    skin_event->u.pen.rubber_pointer =
                            (event->pointerType() == QTabletEvent::Eraser);
#endif  // QT_VERSION
    skin_event->u.pen.x = event->pos().x();
    skin_event->u.pen.y = event->pos().y();
    skin_event->u.pen.x_global = event->globalPos().x();
    skin_event->u.pen.y_global = event->globalPos().y();
    skin_event->u.pen.skip_sync = skipSync;

    queueSkinEvent(skin_event);
}

void EmulatorQtWindow::handleMouseWheelEvent(int delta,
                                             Qt::Orientation orientation) {
    SkinEvent* skin_event = createSkinEvent(kEventMouseWheel);
    skin_event->u.wheel.x_delta = 0;
    skin_event->u.wheel.y_delta = 0;
    if (orientation == Qt::Horizontal) {
        skin_event->u.wheel.x_delta = delta;
    } else {
        skin_event->u.wheel.y_delta = delta;
    }

    queueSkinEvent(skin_event);
}

void EmulatorQtWindow::forwardKeyEventToEmulator(SkinEventType type,
                                                 QKeyEvent* event) {
    SkinEvent* skin_event = createSkinEvent(type);
    SkinEventKeyData& keyData = skin_event->u.key;
    keyData.keycode = convertKeyCode(event->key());
    if (keyData.keycode == -1) {
        D("Failed to convert key for event key %d", event->key());
        delete skin_event;
        return;
    }
    Qt::KeyboardModifiers modifiers = event->modifiers();
    if (modifiers & Qt::ShiftModifier)
        keyData.mod |= kKeyModLShift;
    if (modifiers & Qt::ControlModifier)
        keyData.mod |= kKeyModLCtrl;
    if (modifiers & Qt::AltModifier)
        keyData.mod |= kKeyModLAlt;

    queueSkinEvent(skin_event);
}

void EmulatorQtWindow::handleKeyEvent(SkinEventType type, QKeyEvent* event) {
    // TODO(liyl): Make this shortcut configurable instead of hard-coding it
    // inside the code.
    if (event->key() == Qt::Key_R &&
        event->modifiers() == Qt::ControlModifier && type == kEventKeyDown) {
        D("%s: mouse released\n", __func__);
        releaseMouse();

        SkinEvent* skin_event = createSkinEvent(kEventMouseStopTracking);
        queueSkinEvent(skin_event);

        mMouseGrabbed = false;
    }

    if (!mForwardShortcutsToDevice && mInZoomMode) {
        if (event->key() == Qt::Key_Control) {
            if (type == kEventKeyUp) {
                raise();
                mOverlay.showForZoomUserHidden();
            }
        }
    }

    if (!mForwardShortcutsToDevice && !mInZoomMode &&
        event->key() == Qt::Key_Control &&
            (event->modifiers() == Qt::ControlModifier ||
             event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) )) {
        if (type == kEventKeyDown) {
            if (mToolWindow->getUiEmuAgent()->multiDisplay->isMultiDisplayEnabled() == false) {
                raise();
                D("%s: Using default display for multi touch\n", __FUNCTION__);
                mOverlay.showForMultitouch(
                        event->modifiers() == Qt::ControlModifier,
                        deviceGeometry().center());
            } else {
                QPoint mousePosition = mapFromGlobal(QCursor::pos());
                uint32_t w = 0;
                uint32_t h = 0;
                mToolWindow->getUiEmuAgent()->multiDisplay->getCombinedDisplaySize(&w, &h);
                double widthRatio = (double)geometry().width() / (double)w;
                double heightRatio = (double)geometry().height() / (double)h;
                int pos_x, pos_y, startId = -1;
                uint32_t width, height, id;
                while (mToolWindow->getUiEmuAgent()->multiDisplay->getNextMultiDisplay(startId, &id,
                                                                                       &pos_x, &pos_y,
                                                                                       &width, &height,
                                                                                       nullptr, nullptr,
                                                                                       nullptr)) {
                    QRect r(static_cast<int>(pos_x * widthRatio),
                            static_cast<int>(pos_y * heightRatio),
                            static_cast<int>(width * widthRatio),
                            static_cast<int>(height * heightRatio));
                    if (r.contains(mousePosition, true)) {
                        D("%s: using display %d for multi touch\n",
                          __FUNCTION__, id);
                        mOverlay.showForMultitouch(
                                event->modifiers() == Qt::ControlModifier,
                                r.center());
                        break;
                    }
                    startId = id;
                }
            }
        }
    }

    bool qtEvent = mToolWindow->handleQtKeyEvent(
            event, QtKeyEventSource::EmulatorWindow);

    if (use_keycode_forwarding) {
        return;
    }

    if (mForwardShortcutsToDevice || !qtEvent) {
        forwardKeyEventToEmulator(type, event);
        if (type == kEventKeyDown && event->text().length() > 0) {
            Qt::KeyboardModifiers mods = event->modifiers();
            mods &= ~(Qt::ShiftModifier | Qt::KeypadModifier);
            if (mods == 0) {
                // The key event generated text without Ctrl, Alt, etc.
                // Send an additional TextInput event to the emulator.
                SkinEvent* skin_event = createSkinEvent(kEventTextInput);
                skin_event->u.text.down = false;
                strncpy((char*)skin_event->u.text.text,
                        (const char*)event->text().toUtf8().constData(),
                        sizeof(skin_event->u.text.text) - 1);
                // Ensure the event's text is 0-terminated
                skin_event->u.text.text[sizeof(skin_event->u.text.text) - 1] =
                        0;
                queueSkinEvent(skin_event);
            }
        }
    }
}

void EmulatorQtWindow::simulateKeyPress(int keyCode, int modifiers) {
    SkinEvent* event = createSkinEvent(kEventKeyDown);
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    queueSkinEvent(event);

    event = createSkinEvent(kEventKeyUp);
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    queueSkinEvent(event);
}

void EmulatorQtWindow::simulateScrollBarChanged(int x, int y) {
    SkinEvent* event = createSkinEvent(kEventScrollBarChanged);
    event->u.scroll.x = x;
    event->u.scroll.xmax = mContainer.horizontalScrollBar()->maximum();
    event->u.scroll.y = y;
    event->u.scroll.ymax = mContainer.verticalScrollBar()->maximum();
    queueSkinEvent(event);
}

void EmulatorQtWindow::setDisplayRegion(int xOffset, int yOffset, int width, int height) {
    SkinEvent* event = createSkinEvent(kEventSetDisplayRegion);
    event->u.display_region.xOffset = xOffset;
    event->u.display_region.yOffset = yOffset;
    event->u.display_region.width   = width;
    event->u.display_region.height  = height;
    queueSkinEvent(event);
}

void EmulatorQtWindow::setDisplayRegionAndUpdate(int xOffset, int yOffset, int width, int height) {
    SkinEvent* event = createSkinEvent(kEventSetDisplayRegionAndUpdate);
    event->u.display_region.xOffset = xOffset;
    event->u.display_region.yOffset = yOffset;
    event->u.display_region.width   = width;
    event->u.display_region.height  = height;
    queueSkinEvent(event);
}

void EmulatorQtWindow::simulateSetScale(double scale) {
    // Avoid zoom and scale events clobbering each other if the user rapidly
    // changes zoom levels
    if (mInZoomMode && mNextIsZoom) {
        return;
    }

    // Reset our local copy of zoom factor
    mZoomFactor = 1.0;

    SkinEvent* event = createSkinEvent(kEventSetScale);
    event->u.window.scale = scale;
    queueSkinEvent(event);
}

void EmulatorQtWindow::simulateSetZoom(double zoom) {
    // Avoid zoom and scale events clobbering each other if the user rapidly
    // changes zoom levels
    if (mNextIsZoom || mZoomFactor == zoom) {
        return;
    }

    // Qt Widgets do not get properly sized unless they appear at least once.
    // The scroll bars *must* be properly sized in order for zoom to create the
    // correct GLES subwindow, so this ensures they will be. This is reset as
    // soon as the window is shown.
    mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    mNextIsZoom = true;
    mZoomFactor = zoom;

    QSize viewport = mContainer.viewportSize();

    SkinEvent* event = createSkinEvent(kEventSetZoom);
    event->u.window.x = viewport.width();
    event->u.window.y = viewport.height();

    QScrollBar* horizontal = mContainer.horizontalScrollBar();
    event->u.window.scroll_h =
            horizontal->isVisible() ? horizontal->height() : 0;
    event->u.window.scale = zoom;
    queueSkinEvent(event);
}

void EmulatorQtWindow::simulateWindowMoved(const QPoint& pos) {
    SkinEvent* event = createSkinEvent(kEventWindowMoved);
    event->u.window.x = pos.x();
    event->u.window.y = pos.y();
    queueSkinEvent(event);

    mOverlay.move(mContainer.mapToGlobal(QPoint()));
}

void EmulatorQtWindow::simulateZoomedWindowResized(const QSize& size) {
    mOverlay.resize(size);
    if (!mInZoomMode) {
        return;
    }

    SkinEvent* event = createSkinEvent(kEventZoomedWindowResized);
    QScrollBar* horizontal = mContainer.horizontalScrollBar();
    event->u.scroll.x = horizontal->value();
    event->u.scroll.y = mContainer.verticalScrollBar()->value();
    event->u.scroll.xmax = size.width();
    event->u.scroll.ymax = size.height();
    event->u.scroll.scroll_h =
            horizontal->isVisible() ? horizontal->height() : 0;
    queueSkinEvent(event);
}

void EmulatorQtWindow::setForwardShortcutsToDevice(int index) {
    mForwardShortcutsToDevice = (index != 0);
}

void EmulatorQtWindow::slot_runOnUiThread(RunOnUiThreadFunc f,
                                          QSemaphore* semaphore) {
    QSemaphoreReleaser semReleaser(semaphore);
    f();
}

bool EmulatorQtWindow::hasFrame() const {
    if (mFrameAlways || mInZoomMode) {
        return true;
    }
    // Probably frameless. But framed if there's no skin.
    char *skinName, *skinDir;
    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    return (skinDir == NULL);
}

bool EmulatorQtWindow::isInZoomMode() const {
    return mInZoomMode;
}

ToolWindow* EmulatorQtWindow::toolWindow() const {
    return mToolWindow;
}

CarClusterWindow* EmulatorQtWindow::carClusterWindow() const {
    return mCarClusterWindow;
}

CarClusterConnector* EmulatorQtWindow::carClusterConnector() const {
    return mCarClusterConnector;
}

EmulatorContainer* EmulatorQtWindow::containerWindow() {
    return &mContainer;
}

void EmulatorQtWindow::showZoomIfNotUserHidden() {
    if (!mOverlay.wasZoomUserHidden()) {
        mOverlay.showForZoom();
    }
}

QSize EmulatorQtWindow::containerSize() const {
    return mContainer.size();
}

QRect EmulatorQtWindow::deviceGeometry() const {
    return mDeviceGeometry;
}

android::emulation::AdbInterface* EmulatorQtWindow::getAdbInterface() const {
    return (*mAdbInterface);
}

void EmulatorQtWindow::toggleZoomMode() {
    mInZoomMode = !mInZoomMode;

    // Exiting zoom mode snaps back to aspect ratio
    if (!mInZoomMode) {
        // Scroll bars should be turned off immediately.
        mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        doResize(mContainer.size());
        mOverlay.hide();
    } else {
        doResize(mContainer.size());
        mOverlay.showForZoom();
    }
    maskWindowFrame();
}

void EmulatorQtWindow::recenterFocusPoint() {
    mContainer.horizontalScrollBar()->setValue(mFocus.x() * width() -
                                               mViewportFocus.x());
    mContainer.verticalScrollBar()->setValue(mFocus.y() * height() -
                                             mViewportFocus.y());

    mFocus = QPointF();
    mViewportFocus = QPoint();
}

void EmulatorQtWindow::saveZoomPoints(const QPoint& focus,
                                      const QPoint& viewportFocus) {
    // The underlying frame will change sizes, so get what "percentage" of the
    // frame was clicked, where (0,0) is the top-left corner and (1,1) is the
    // bottom right corner.
    mFocus = QPointF((float)focus.x() / this->width(),
                     (float)focus.y() / this->height());

    // Save to re-align the container with the underlying frame.
    mViewportFocus = viewportFocus;
}

void EmulatorQtWindow::scaleDown() {
    doResize(mContainer.size() / 1.1, true);
}

void EmulatorQtWindow::scaleUp() {
    doResize(mContainer.size() * 1.1, true);
}

void EmulatorQtWindow::fixScale() {
    doResize(mContainer.size(), true);
}

void EmulatorQtWindow::zoomIn() {
    zoomIn(QPoint(width() / 2, height() / 2),
           QPoint(mContainer.width() / 2, mContainer.height() / 2));
}

void EmulatorQtWindow::zoomIn(const QPoint& focus,
                              const QPoint& viewportFocus) {
    if (!mBackingSurface)
        return;

    saveZoomPoints(focus, viewportFocus);

    // The below scale = x creates a skin equivalent to calling "window
    // scale x" through the emulator console. At scale = 1, the device
    // should be at a 1:1 pixel mapping with the monitor. We allow going
    // to twice this size.
    double scale =
            ((double)size().width() / (double)mBackingSurface->bitmap->size().width());
    double maxZoom = mZoomFactor * 2.0 / scale;

    if (scale < 2) {
        simulateSetZoom(std::min(mZoomFactor + .25, maxZoom));
    }
}

void EmulatorQtWindow::zoomOut() {
    zoomOut(QPoint(width() / 2, height() / 2),
            QPoint(mContainer.width() / 2, mContainer.height() / 2));
}

void EmulatorQtWindow::zoomOut(const QPoint& focus,
                               const QPoint& viewportFocus) {
    saveZoomPoints(focus, viewportFocus);
    if (mZoomFactor > 1) {
        simulateSetZoom(std::max(mZoomFactor - .25, 1.0));
    }
}

void EmulatorQtWindow::zoomReset() {
    simulateSetZoom(1);
}

void EmulatorQtWindow::zoomTo(const QPoint& focus, const QSize& rectSize) {
    if (!mBackingSurface)
        return;

    saveZoomPoints(focus,
                   QPoint(mContainer.width() / 2, mContainer.height() / 2));

    // The below scale = x creates a skin equivalent to calling "window
    // scale x" through the emulator console. At scale = 1, the device
    // should be at a 1:1 pixel mapping with the monitor. We allow going to
    // twice this size.
    double scale =
            ((double)size().width() / (double)mBackingSurface->bitmap->size().width());

    // Calculate the "ideal" zoom factor, which would perfectly frame this
    // rectangle, and the "maximum" zoom factor, which makes scale = 1, and
    // pick the smaller one. Adding 20 accounts for the scroll bars
    // potentially cutting off parts of the selection
    double maxZoom = mZoomFactor * 2.0 / scale;
    double idealWidthZoom = mZoomFactor * (double)mContainer.width() /
                            (double)(rectSize.width() + 20);
    double idealHeightZoom = mZoomFactor * (double)mContainer.height() /
                             (double)(rectSize.height() + 20);

    simulateSetZoom(std::min({idealWidthZoom, idealHeightZoom, maxZoom}));
}

void EmulatorQtWindow::panHorizontal(bool left) {
    QScrollBar* bar = mContainer.horizontalScrollBar();
    if (left) {
        bar->setValue(bar->value() - bar->singleStep());
    } else {
        bar->setValue(bar->value() + bar->singleStep());
    }
}

void EmulatorQtWindow::panVertical(bool up) {
    QScrollBar* bar = mContainer.verticalScrollBar();
    if (up) {
        bar->setValue(bar->value() - bar->singleStep());
    } else {
        bar->setValue(bar->value() + bar->singleStep());
    }
}

bool EmulatorQtWindow::mouseInside() {
    QPoint widget_cursor_coords = mapFromGlobal(QCursor::pos());
    return widget_cursor_coords.x() >= 0 &&
           widget_cursor_coords.x() < width() &&
           widget_cursor_coords.y() >= 0 && widget_cursor_coords.y() < height();
}

void EmulatorQtWindow::wheelEvent(QWheelEvent* event) {
    if (mIgnoreWheelEvent) {
        event->ignore();
    } else if (android::featurecontrol::isEnabled(
                       android::featurecontrol::VirtioMouse)) {
        if (mMouseGrabbed) {
#if QT_VERSION >= 0x060000
            handleMouseWheelEvent(event->angleDelta().y() / 8, Qt::Orientation::Vertical);
            handleMouseWheelEvent(event->angleDelta().x() / 8, Qt::Orientation::Horizontal);
#else
            handleMouseWheelEvent(event->delta() / 8, event->orientation());
#endif  // QT_VERSION
        }
    } else {
        if (!mWheelScrollTimer.isActive()) {
#if QT_VERSION >= 0x060000
            handleMouseEvent(kEventMouseButtonDown, kMouseButtonLeft,
                             event->position(), QPointF(0.0, 0.0));
            mWheelScrollPos = event->position();
            mWheelScrollTimer.start();
            mWheelScrollPos.setY(mWheelScrollPos.y() + event->angleDelta().y() / 8);
            handleMouseEvent(kEventMouseMotion, kMouseButtonLeft, mWheelScrollPos,
                             QPointF(0.0, 0.0));
#else
            handleMouseEvent(kEventMouseButtonDown, kMouseButtonLeft,
                             event->pos(), QPoint(0, 0));
            mWheelScrollPos = event->pos();
            mWheelScrollTimer.start();
            mWheelScrollPos.setY(mWheelScrollPos.y() + event->delta() / 8);
            handleMouseEvent(kEventMouseMotion, kMouseButtonLeft, mWheelScrollPos,
                             QPoint(0, 0));
#endif  // QT_VERSION
        }
    }
}

void EmulatorQtWindow::wheelScrollTimeout() {
#if QT_VERSION >= 0x060000
    handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft, mWheelScrollPos, QPointF(0.0 ,0.0));
#else
    handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft, mWheelScrollPos, QPoint(0,0));
#endif  // QT_VERSION
}

void EmulatorQtWindow::checkAdbVersionAndWarn() {
    // Do not check for ADB in Fuchsia mode.
    if (is_fuchsia) return;

    QSettings settings;
    if (!(*mAdbInterface)->isAdbVersionCurrent() &&
        settings.value(Ui::Settings::AUTO_FIND_ADB, true).toBool()) {
        std::string adb_path = (*mAdbInterface)->detectedAdbPath();
        if (adb_path.empty()) {
            mAdbWarningBox->setText(
                    tr("Could not automatically detect an ADB binary. Some "
                       "emulator functionality will not work until a custom "
                       "path to ADB  is added. "
                       "This can be done in Extended Controls (...) > Settings "
                       "> General tab > 'Use detected ADB location'"));
        } else {
            mAdbWarningBox->setText(
                    tr("The ADB binary found at %1 is obsolete and has serious"
                       "performance problems with the Android Emulator. Please "
                       "update to a newer version to get significantly faster "
                       "app/file transfer.")
                            .arg(adb_path.c_str()));
        }
        QSettings settings;
        if (settings.value(Ui::Settings::SHOW_ADB_WARNING, true).toBool()) {
            QObject::connect(mAdbWarningBox.ptr(),
                             SIGNAL(buttonClicked(QAbstractButton*)), this,
                             SLOT(slot_adbWarningMessageAccepted()));

            QCheckBox* checkbox = new QCheckBox(tr("Never show this again."));
            checkbox->setCheckState(Qt::Unchecked);
            mAdbWarningBox->setWindowModality(Qt::NonModal);
            mAdbWarningBox->setCheckBox(checkbox);
            mAdbWarningBox->show();
        }
    }
}

void EmulatorQtWindow::slot_adbWarningMessageAccepted() {
    QCheckBox* checkbox = mAdbWarningBox->checkBox();
    if (checkbox->checkState() == Qt::Checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_ADB_WARNING, false);
    }
}

void EmulatorQtWindow::runAdbShellPowerDownAndQuit() {
    // we need to run it only once, so don't ever reset this
    if (mStartedAdbStopProcess) {
        return;
    }
    mStartedAdbStopProcess = true;
    (*mAdbInterface)->runAdbCommand(
            {"shell", "stop"},
            [this](const android::emulation::OptionalAdbCommandResult&) {
                queueQuitEvent();
            },
            5000); // for qemu1, reboot -p will shutdown guest but hangs, allow 5s
}

#ifdef _WIN32
void EmulatorQtWindow::checkVgkAndWarn() {
    AndroidCpuAccelerator accelerator = androidCpuAcceleration_getAccelerator();

    if (accelerator != ANDROID_CPU_ACCELERATOR_HAX &&
        accelerator != ANDROID_CPU_ACCELERATOR_GVM)
        return;

    if (::android::base::Win32Utils::getServiceStatus("vgk") <= SVC_NOT_FOUND)
        return;

    QSettings settings;
    if (settings.value(Ui::Settings::SHOW_VGK_WARNING, true).toBool()) {
        QObject::connect(mVgkWarningBox.ptr(),
                         SIGNAL(buttonClicked(QAbstractButton*)), this,
                         SLOT(slot_vgkWarningMessageAccepted()));

        QCheckBox* checkbox = new QCheckBox(tr("Never show this again."));
        checkbox->setCheckState(Qt::Unchecked);
        mVgkWarningBox->setWindowModality(Qt::NonModal);
        mVgkWarningBox->setCheckBox(checkbox);
        mVgkWarningBox->show();
    }
}

void EmulatorQtWindow::slot_vgkWarningMessageAccepted() {
    QCheckBox* checkbox = mVgkWarningBox->checkBox();
    if (checkbox->checkState() == Qt::Checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_VGK_WARNING, false);
    }
}
#endif

void EmulatorQtWindow::checkNestedAndWarn() {
    char hv_vendor_id[16] = {};
    android_get_x86_cpuid_vmhost_vendor_id(hv_vendor_id, sizeof(hv_vendor_id));
    auto vmhost = android_get_x86_cpuid_vendor_vmhost_type(hv_vendor_id);
    // Emulator running on native HW
    if (vmhost == VENDOR_VM_NOTVM) {
        return;
    }
    // Emulator running with software emulation
    AndroidCpuAccelerator accelerator = androidCpuAcceleration_getAccelerator();
    if (accelerator == ANDROID_CPU_ACCELERATOR_NONE) {
        return;
    }
    /* Emulator running on hyper-v hypervisor
     * Typically, when hypervisor is detected, emulator will run nested
     * virtualization. However, for type-1 hypervisor, detecting hypervisor
     * could only mean we are on an "admin" guest (or host as is usually
     * called). In these cases, emulator may actually create a "non-admin"
     * guest and run with it. This is NOT nested virtualization.
     *
     * TODO: Xen??
     */
    if (vmhost == VENDOR_VM_HYPERV &&
        accelerator == ANDROID_CPU_ACCELERATOR_WHPX) {
        return;
    }
    if (android_cmdLineOptions->no_nested_warnings) {
        return;
    }
    QSettings settings;
    if (settings.value(Ui::Settings::SHOW_NESTED_WARNING, true).toBool()) {
        QObject::connect(mNestedWarningBox.ptr(),
                         SIGNAL(buttonClicked(QAbstractButton*)), this,
                         SLOT(slot_nestedWarningMessageAccepted()));

        QCheckBox* checkbox = new QCheckBox(tr("Never show this again."));
        checkbox->setCheckState(Qt::Unchecked);
        mNestedWarningBox->setWindowModality(Qt::NonModal);
        mNestedWarningBox->setCheckBox(checkbox);
        mNestedWarningBox->show();
    }
}

void EmulatorQtWindow::slot_nestedWarningMessageAccepted() {
    QCheckBox* checkbox = mNestedWarningBox->checkBox();
    if (checkbox->checkState() == Qt::Checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_NESTED_WARNING, false);
    }
}

void EmulatorQtWindow::rotateSkin(SkinRotation rot) {
    // Hack. Notify the parent container that we're rotating, so it doesn't
    // start a regular scaling timer: we know that the scale is correct as
    // it was correct before the rotation.
    mOrientation = rot;
    mContainer.prepareForRotation();

    SkinEvent* event = createSkinEvent(kEventLayoutRotate);
    event->u.layout_rotation.rotation = rot;
    queueSkinEvent(event);

    if (android_foldable_is_folded()) {
        resizeAndChangeAspectRatio(true);
    }

    if (resizableEnabled()) {
        PresetEmulatorSizeInfo info;
        if (getResizableConfig(getResizableActiveConfigId(), &info)) {
            resizeAndChangeAspectRatio(0, 0, info.width, info.height);
        }
    }

#ifdef __APPLE__
    // To fix issues when resizing + linking against macos sdk 11.
    SkinEvent* changeEvent = new SkinEvent();
    changeEvent->type = kEventScreenChanged;
    queueSkinEvent(changeEvent);
#endif
}

void EmulatorQtWindow::setVisibleExtent(QBitmap bitMap) {
    QImage bitImage = bitMap.toImage();
    int topVisible = -1;
    for (int row = 0; row < bitImage.height() && topVisible < 0; row++) {
        for (int col = 0; col < bitImage.width(); col++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                topVisible = row;
                break;
            }
        }
    }
    int bottomVisible = -1;
    for (int row = bitImage.height() - 1; row >= 0 && bottomVisible < 0; row--) {
        for (int col = 0; col < bitImage.width(); col++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                bottomVisible = row;
                break;
            }
        }
    }

    int leftVisible = -1;
    for (int col = 0; col < bitImage.width() && leftVisible < 0; col++) {
        for (int row = 0; row < bitImage.height(); row++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                leftVisible = col;
                break;
            }
        }
    }
    int rightVisible = -1;
    for (int col = bitImage.width() - 1; col >= 0 && rightVisible < 0; col--) {
        for (int row = 0; row < bitImage.height(); row++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                rightVisible = col;
                break;
            }
        }
    }

    if (topVisible    < 0 || rightVisible < 0 ||
        bottomVisible < 0 || leftVisible  < 0   )
    {
        mSkinGapTop = 0;
        mSkinGapRight = 0;
        mSkinGapBottom = 0;
        mSkinGapLeft = 0;
    } else {
        mSkinGapTop = topVisible;
        mSkinGapRight = bitImage.width() - 1 - rightVisible;
        mSkinGapBottom = bitImage.height() - 1 - bottomVisible;
        mSkinGapLeft = leftVisible;
    }
}

void EmulatorQtWindow::updateUIMultiDisplayPage(uint32_t id) {
    // update the MutiDisplay UI page, e.g., config through console,
    // loaded from snapshot
    if (id > 0 && id <= MultiDisplayPage::sMaxItem) {
        emit(updateMultiDisplayPage(id));
    }
}

bool EmulatorQtWindow::getMultiDisplay(uint32_t id,
                                       int32_t* x,
                                       int32_t* y,
                                       uint32_t* w,
                                       uint32_t* h,
                                       uint32_t* dpi,
                                       uint32_t* flag,
                                       bool* enabled) {
    const auto uiAgent = mToolWindow->getUiEmuAgent();
    return (uiAgent != nullptr &&
            uiAgent->multiDisplay != nullptr &&
            uiAgent->multiDisplay->getMultiDisplay(id, x, y, w, h, dpi, flag, enabled));
}


int EmulatorQtWindow::switchMultiDisplay(bool enabled,
                                          uint32_t id,
                                          int32_t x,
                                          int32_t y,
                                          uint32_t width,
                                          uint32_t height,
                                          uint32_t dpi,
                                          uint32_t flag) {
    const auto uiAgent = mToolWindow->getUiEmuAgent();
    return (uiAgent != nullptr &&
            uiAgent->multiDisplay != nullptr &&
            uiAgent->multiDisplay->setMultiDisplay(id, x, y, width, height, dpi, flag, enabled));
}

bool EmulatorQtWindow::getMonitorRect(uint32_t* width, uint32_t* height) {
    SkinRect monitor;
    skin_winsys_get_monitor_rect(&monitor);
    if (width) {
        *width = monitor.size.w;
    }
    if (height) {
        *height = monitor.size.h;
    }
    return true;
}

void EmulatorQtWindow::setNoSkin() {
    runOnUiThread([this]() {
        char *skinName, *skinDir;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinDir != NULL) {
            SkinEvent* event = new SkinEvent();
            event->type = kEventSetNoSkin;
            skin_event_add(event);
        }
        mToolWindow->hideRotationButton(true);
        setFrameAlways(true);
    });
}

void EmulatorQtWindow::restoreSkin() {
    runOnUiThread([this]() {
        char *skinName, *skinDir;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinDir != NULL) {
            SkinEvent* event = new SkinEvent();
            event->type = kEventRestoreSkin;
            skin_event_add(event);
        }
        mToolWindow->hideRotationButton(false);
        setFrameAlways(false);
    });
}

void EmulatorQtWindow::setUIDisplayRegion(int x, int y, int w, int h, bool ignoreOrientation) {
    runOnUiThread([this, x, y, w, h, ignoreOrientation]() {this->resizeAndChangeAspectRatio(x, y, w, h, ignoreOrientation);});
}

bool EmulatorQtWindow::addMultiDisplayWindow(uint32_t id, bool add, uint32_t w, uint32_t h) {
    SkinEvent* skin_event = nullptr;
    if (add) {
        QRect windowGeo = this->geometry();
        if (!mBackingSurface) {
            LOG(ERROR) << "backing surface not ready, cancel window creation";
            return false;
        }
        skin_event = createSkinEvent(kEventAddDisplay);
        skin_event->u.add_display.width = w;
        skin_event->u.add_display.height = h;
        skin_event->u.add_display.id = id;
        QSize backingSize = mBackingSurface->bitmap->size();
        float scale = (float)windowGeo.width() / (float)backingSize.width();
        if (mMultiDisplayWindow[id] == nullptr) {
            // create qt window for the 1st time.
            mMultiDisplayWindow[id].reset(new MultiDisplayWidget(w, h, id));
            char title[16];
            snprintf(title, 16, "Display %d", id);
            mMultiDisplayWindow[id]->setWindowTitle(title);
            QRect geoTool = mToolWindow->geometry();
            mMultiDisplayWindow[id]->move(geoTool.right() + (mMultiDisplayWindow.size() - 1) * 100,
                                          geoTool.top());
        }
        mMultiDisplayWindow[id]->setMouseTracking(true);
        mMultiDisplayWindow[id]->setFixedSize(QSize((int)(w * scale), (int)(h * scale)));
        mMultiDisplayWindow[id]->show();
    } else {
        skin_event = createSkinEvent(kEventRemoveDisplay);
        skin_event->u.remove_display.id = id;
        auto iter = mMultiDisplayWindow.find(id);
        if (iter != mMultiDisplayWindow.end()) {
            mMultiDisplayWindow.erase(iter);
        }
    }
    queueSkinEvent(skin_event);
    return true;
}

bool EmulatorQtWindow::paintMultiDisplayWindow(uint32_t id, uint32_t texture) {
    if (mMultiDisplayWindow[id] == nullptr || mMultiDisplayWindow[id]->isHidden()) {
        return true;
    }
    mMultiDisplayWindow[id]->paintWindow(texture);
    return true;
}

bool EmulatorQtWindow::multiDisplayParamValidate(uint32_t id, uint32_t w, uint32_t h,
                                                 uint32_t dpi, uint32_t flag) {
    const auto uiAgent = mToolWindow->getUiEmuAgent();
    return uiAgent->multiDisplay->multiDisplayParamValidate(id, w, h, dpi, flag);
}

void EmulatorQtWindow::saveMultidisplayToConfig() {
    int pos_x, pos_y;
    uint32_t width, height, dpi, flag;
    const int maxEntries = avdInfo_maxMultiDisplayEntries();
    for (uint32_t i = 1; i < maxEntries + 1; i++) {
        if (!getMultiDisplay(i, &pos_x, &pos_y, &width, &height, &dpi, &flag, nullptr)) {
            avdInfo_replaceMultiDisplayInConfigIni(android_avdInfo,
                                                   i,
                                                   -1,
                                                   -1,
                                                   0,
                                                   0,
                                                   0,
                                                   0);
        } else  {
            avdInfo_replaceMultiDisplayInConfigIni(android_avdInfo,
                                                   i,
                                                   pos_x,
                                                   pos_y,
                                                   width,
                                                   height,
                                                   dpi,
                                                   flag);
        }
    }
}

/* Convert from tilt coordinates to rotation of pen
 * xTilt: the plane angle (in degrees, range [-60,60]) between
 * the Y-Z plane and the plane containing both the pen and the Y axis
 *      positive values are towards the screen's physical right
 * yTilt: the plane angle (in degrees, range [-60,60]) between
 * the X-Z plane and the plane containing both the pen and the X axis
 *      positive values are towards the bottom of the screen (user)
 * rotation: the azimuth angle (in degrees, range (-180,180]) of the pen,
 * where 0 represents a pen whose cap is pointing down (tip points up),
 * +90 to the left, -90 to the right, 180 cap points up, clockwise.
 * When the pen is perpendicular to the screen the rotation is 0. */
int EmulatorQtWindow::tiltToRotation(int xTiltDeg, int yTiltDeg) {
    int rotationDeg = 0;
    // The equations used, in an X-Y-Z coordinate system:
    // tan(X Tilt) = X / Z
    // tan(Y Tilt) = Y / Z
    // tan(azimuth) = Y / X = tan(Y Tilt) / tan(X Tilt)
    // azimuth = arctan( tan(Y Tilt) / tan(X Tilt) )

    if ((xTiltDeg != 0) && (yTiltDeg != 0)) {
        // domain error may occur if both arguments x,y of atan2 are 0

        // pen is not parallel to any axis, nor perpendicular to screen
        // use the south-clockwise convention for azimuth: (-X, -Y)
        // the Y axis is already reversed on a screen (down, not up)
        double xTiltRad = -qDegreesToRadians((double)xTiltDeg);
        double yTiltRad =  qDegreesToRadians((double)yTiltDeg);

        // atan2 function computes the arc tangent of y/x using the signs
        // of arguments to determine the correct quadrant
        rotationDeg = (int)qRadiansToDegrees(std::atan2(std::tan(xTiltRad),
                                                        std::tan(yTiltRad)));
    } else if (xTiltDeg != 0) {
        // pen is parallel to the X axis, not perpendicular to screen
        rotationDeg = (xTiltDeg > 0) ? -90 : 90;
    } else {
        // pen is parallel to the Y axis, or perpendicular to screen
        rotationDeg = (yTiltDeg >= 0) ? 0 : 180;
    }

    return rotationDeg;
}

// Adapt the rotation angle depending on screen rotation
int EmulatorQtWindow::penOrientation(int rotation) {
    int orientation = rotation;

    switch (mOrientation) {
    case SKIN_ROTATION_0:
        break;
    case SKIN_ROTATION_90:
        orientation -= 90;
        break;
    case SKIN_ROTATION_180:
        orientation -= 180;
        break;
    case SKIN_ROTATION_270:
        orientation -= 270;
        break;
    default:
        break;
    }
    // keep the orientation in the range (-180, 180] degrees
    if (orientation <= -180) {
        orientation += 360;
    }

    return orientation;
}

// State machine that translates the pen event types based on button states
// If during a touching state the button is pressed this generates unwanted
// Press and Release events which are translated to Move events
SkinEventType EmulatorQtWindow::translatePenEventType(SkinEventType type,
                                                    Qt::MouseButton button,
                                                    Qt::MouseButtons buttons) {
    SkinEventType newType = type;

    switch (mPenTouchState) {
    case TouchState::NOT_TOUCHING:
        // Only the first Press event can have the same pressed buttons
        // button:  only the button that caused the event
        // buttons: all the pressed buttons
        if ((type == kEventPenPress) && (button == buttons)) {
            mPenTouchState = TouchState::TOUCHING;
            newType = kEventPenPress;
        } else {
            newType = kEventPenRelease;
        }
        break;
    case TouchState::TOUCHING:
        // Only the last Release event can have no pressed buttons
        if ((type == kEventPenRelease) && (buttons == Qt::NoButton)) {
            mPenTouchState = TouchState::NOT_TOUCHING;
            newType = kEventPenRelease;
        } else {
            newType = kEventPenMove;
        }
        break;
    default:
        mPenTouchState = TouchState::NOT_TOUCHING;
        newType = kEventPenRelease;
        break;
    }

    return newType;
}

// State machine that translates the mouse event types based on button states
// If during a touching state the button is pressed this generates unwanted
// Press and Release events which are translated to Move events
SkinEventType EmulatorQtWindow::translateMouseEventType(SkinEventType type,
                                                    Qt::MouseButton button,
                                                    Qt::MouseButtons buttons) {
    SkinEventType newType = type;

    switch (mMouseTouchState) {
    case TouchState::NOT_TOUCHING:
        // Only the first Press event can have the same pressed buttons
        // button:  only the button that caused the event
        // buttons: all the pressed buttons
        if ((type == kEventMouseButtonDown) && (button == buttons)) {
            mMouseTouchState = TouchState::TOUCHING;
            newType = kEventMouseButtonDown;
        } else {
            newType = kEventMouseButtonUp;
        }
        break;
    case TouchState::TOUCHING:
        // Only the last Release event can have no pressed buttons
        if ((type == kEventMouseButtonUp) && (buttons == Qt::NoButton)) {
            mMouseTouchState = TouchState::NOT_TOUCHING;
            newType = kEventMouseButtonUp;
        } else {
            newType = kEventMouseMotion;
        }
        break;
    default:
        mMouseTouchState = TouchState::NOT_TOUCHING;
        newType = kEventMouseButtonUp;
        break;
    }

    return newType;
}
