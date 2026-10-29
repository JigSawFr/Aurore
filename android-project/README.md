# Aurore on Android

[中文](./README_ZH.md)

The Gradle project that builds Aurore's demo as an Android app. It follows SDL's Android port ([SDL 2.32's documentation](https://github.com/libsdl-org/SDL/blob/release-2.32.x/docs/README-android.md), [SDL 3.4's](https://github.com/libsdl-org/SDL/blob/release-3.4.x/docs/README-android.md)): SDL's Java activity starts the app, which Aurore builds as a shared library with the SDL platform and OpenGL ES 2, its resources inside it (libromfs). It is built on SDL2, or on SDL3 with `-PauroreSdl=3` (or `auroreSdl=3` in `gradle.properties`).

| | |
| --- | --- |
| SDL2 | 2.32.10, the submodule `library/lib/extern/SDL` (`app/jni/SDL` links to it). The Java code in `app/src/sdl2/java/org/libsdl/app` is SDL's from the same tag (`SDLActivity` refuses a native library of another version), with one change: `HIDDeviceManager` registers its USB receiver with `RECEIVER_EXPORTED`, which Android 14 requires |
| SDL3 | 3.4.18, the submodule `library/lib/extern/SDL3` (`app/jni/SDL3` links to it), with `-PauroreSdl=3`. Its Java code is read from the submodule as it is (`android-project/app/src/main/java` there) |
| Aurore's Java | `app/src/main/java/org/libsdl/app`: `PlatformUtils` and `AuroreHandler`, which the library calls (brightness, network, the system's language), for either SDL |
| Android Gradle Plugin, Gradle | 9.5.0, Gradle 9.8.2 (the wrapper, `gradlew`) |
| JDK | 17 or later, as the Android Gradle Plugin 9 needs |
| NDK | r30, 30.0.16248370 (`ndkVersion` in `app/build.gradle`): 16 KB pages, as Google Play asks |
| CMake | 4.1.2, from the SDK (`externalNativeBuild` in `app/build.gradle`) |
| SDK | compiled against and targeting API 36 (Android 16), what Google Play asks of new apps and updates; `minSdk 21` (Android 5.0) |
| ABIs | `armeabi-v7a`, `arm64-v8a` |

The NDK sets the oldest Android the app runs on: r30 goes down to API 21. Targeting API 36 turns Android 16's predictive back on, which no longer sends the back key to the app: `android:enableOnBackInvokedCallback="false"` in `AndroidManifest.xml` keeps it reaching SDL, which reads it as the B button. The CI builds the debug APK with the versions above.

The app's data is left out of Android's backups and of transfers to a new device: `android:allowBackup="false"` in `AndroidManifest.xml`, and `res/xml/data_extraction_rules.xml` for Android 12 and later. An app that wants its settings backed up includes those files there.

## Building

From the repository's root, the resources generator first: libromfs turns the resources into C++ sources while the app is built, so it has to run on the computer that builds, not on Android.

```sh
./build_libromfs_generator.sh
```

It leaves `libromfs-generator` at the root of the repository, where `app/jni/CMakeLists.txt` finds it (`app/jni/aurore` is a link to the root). Then, with the Android SDK and NDK installed (Android Studio's SDK Manager, or `sdkmanager`):

```sh
cd android-project
export ANDROID_SDK_ROOT=~/Android/Sdk          # macOS: ~/Library/Android/sdk
./gradlew assembleDebug                        # app/build/outputs/apk/debug/*.apk
./gradlew assembleDebug -PauroreSdl=3          # the same on SDL3
./gradlew installDebug                         # onto a device or emulator connected with adb
```

Android Studio opens `android-project/` as it is. On macOS, its own JDK works: `export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"`.

A release build (`./gradlew assembleRelease`) is signed with `app/aurore.jks`, which is not in the repository: make one (the `keytool` line in `app/build.gradle`), or point `signingConfigs.release` at your own key.

## Your app on Android

1. Put your project in `app/jni/` (or make `app/jni/aurore` a link to your project's root instead of Aurore's), with Aurore as its submodule.
2. In `app/jni/CMakeLists.txt`, `add_subdirectory` your project in place of `aurore`, and point `LIBROMFS_PREBUILT_GENERATOR` at the `libromfs-generator` your build made.
3. In your `CMakeLists.txt`, make the program with `program_target()` (a shared library on Android) and give it your resources with `add_libromfs()`, as Aurore's root `CMakeLists.txt` does for the demo.
4. In `app/build.gradle`, set `namespace` and `applicationId` to your package name.
5. In `app/src/main/java/`, copy `com/aurore/demo/DemoActivity.java` under your package, and change `getLibraries()` to load SDL (`BuildConfig.SDL_LIBRARY`: `SDL2` or `SDL3`) and your library (your CMake target's name, in place of `aurore_demo`).
6. In `app/src/main/AndroidManifest.xml`, name your activity in place of `DemoActivity`.
7. Set the app's name in `app/src/main/res/values/strings.xml` and its icon in `app/src/main/res/mipmap-*/ic_launcher.png`.

The activity calls `System.exit(0)` when it is destroyed: Aurore keeps its state in static variables, which SDL would otherwise reuse when Android creates the activity again.

## Known issues

When a resource file is deleted, libromfs may not notice and the build fails. Running CMake again fixes it: change something in `app/jni/CMakeLists.txt` (an empty line), or remove `app/.cxx`.
