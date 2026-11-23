QT += core gui qml quick quickcontrols2 svg network

CONFIG += c++17 release
TARGET = pengu
TEMPLATE = app

include(src/sources.pri)
SOURCES += src/main.cpp

android {
    # Android differences live in android/ (manifest, icons) and behind
    # Q_OS_ANDROID in C++; the QML is the same everywhere.
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    ANDROID_VERSION_CODE = $$PENGU_BUILD
    ANDROID_VERSION_NAME = $$PENGU_VERSION
    ANDROID_MIN_SDK_VERSION = 28
    ANDROID_TARGET_SDK_VERSION = 36
    # Qt's network stack needs OpenSSL bundled on Android (KDAB's builds).
    OPENSSL_DIR = $$(ANDROID_OPENSSL)
    isEmpty(OPENSSL_DIR): OPENSSL_DIR = $$PWD/toolchain/android_openssl
    exists($$OPENSSL_DIR/ssl_3/arm64-v8a/libssl_3.so) {
        ANDROID_EXTRA_LIBS += $$OPENSSL_DIR/ssl_3/arm64-v8a/libcrypto_3.so \
                              $$OPENSSL_DIR/ssl_3/arm64-v8a/libssl_3.so
    } else {
        warning("android_openssl not found; update checks will fail. Run bin/android-setup.")
    }
}
