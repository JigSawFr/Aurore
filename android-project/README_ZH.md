# Aurore 的安卓版本

[English](./README.md)

本目录是将 Aurore 演示程序构建为安卓应用的 Gradle 工程。它遵循 SDL 的安卓移植方式（[SDL 2.32 文档](https://github.com/libsdl-org/SDL/blob/release-2.32.x/docs/README-android.md)）：由 SDL 的 Java Activity 启动应用，Aurore 以共享库的形式构建，使用 SDL2 平台和 OpenGL ES 2，资源文件通过 libromfs 打包进库中。

| | |
| --- | --- |
| SDL | 2.32.10，子模块 `library/lib/extern/SDL`（`app/jni/SDL` 是指向它的链接）。`app/src/main/java/org/libsdl/app` 中的 Java 代码取自同一标签的 SDL（`SDLActivity` 拒绝其他版本的原生库），只改了一处：`HIDDeviceManager` 以 `RECEIVER_EXPORTED` 注册 USB 广播接收器，这是安卓 14 的要求 |
| Android Gradle Plugin、Gradle | 9.5.0，Gradle 9.8.2（使用 `gradlew`） |
| JDK | 17 或更高，Android Gradle Plugin 9 的要求 |
| NDK | r30，30.0.16248370（`app/build.gradle` 中的 `ndkVersion`）：支持 16 KB 内存页，Google Play 的要求 |
| CMake | 4.1.2，来自 SDK（`app/build.gradle` 中的 `externalNativeBuild`） |
| SDK | 基于并面向 API 36（Android 16）编译，这是 Google Play 对新应用和更新的要求；`minSdk 21`（Android 5.0） |
| ABI | `armeabi-v7a`、`arm64-v8a` |

NDK 决定应用能运行的最低安卓版本：r30 支持到 API 21。面向 API 36 会开启 Android 16 的预测性返回，系统不再把返回键发给应用：`AndroidManifest.xml` 中的 `android:enableOnBackInvokedCallback="false"` 让返回键继续交给 SDL（作为 B 键）。CI 使用上表中的版本构建 debug APK。

应用数据不进入安卓的备份，也不随设备迁移：`AndroidManifest.xml` 中的 `android:allowBackup="false"`，以及适用于 Android 12 及以后版本的 `res/xml/data_extraction_rules.xml`。需要备份设置的应用可以在其中包含相应文件。

## 构建

先在仓库根目录构建资源生成器：libromfs 在编译时把资源文件转换为 C++ 源码，因此它需要在执行构建的电脑上运行，而不是在安卓上。

```sh
./build_libromfs_generator.sh
```

生成的 `libromfs-generator` 位于仓库根目录，`app/jni/CMakeLists.txt` 会在那里找到它（`app/jni/aurore` 是指向根目录的链接）。然后在安装好 Android SDK 和 NDK 之后（通过 Android Studio 的 SDK Manager 或 `sdkmanager`）：

```sh
cd android-project
export ANDROID_SDK_ROOT=~/Android/Sdk          # macOS: ~/Library/Android/sdk
./gradlew assembleDebug                        # app/build/outputs/apk/debug/*.apk
./gradlew installDebug                         # 安装到通过 adb 连接的设备或模拟器
```

也可以直接用 Android Studio 打开 `android-project/`。在 macOS 上可以使用其自带的 JDK：`export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"`。

release 构建（`./gradlew assembleRelease`）使用 `app/aurore.jks` 签名，该文件不在仓库中：请自行生成（参见 `app/build.gradle` 中的 `keytool` 命令），或让 `signingConfigs.release` 指向你自己的密钥。

## 将你的应用移植到安卓

1. 把你的项目放到 `app/jni/` 下（或让 `app/jni/aurore` 指向你的项目根目录而不是 Aurore），Aurore 作为它的子模块。
2. 在 `app/jni/CMakeLists.txt` 中用 `add_subdirectory` 添加你的项目以替代 `aurore`，并让 `LIBROMFS_PREBUILT_GENERATOR` 指向你构建出的 `libromfs-generator`。
3. 在你的 `CMakeLists.txt` 中用 `program_target()` 创建程序（在安卓上是共享库），并用 `add_libromfs()` 打包你的资源，参考 Aurore 根目录 `CMakeLists.txt` 对演示程序的做法。
4. 在 `app/build.gradle` 中把 `namespace` 和 `applicationId` 改为你的包名。
5. 在 `app/src/main/java/` 中，把 `com/aurore/demo/DemoActivity.java` 复制到你的包下，并修改 `getLibraries()`，加载 `SDL2` 和你的库（你的 CMake 目标名，替代 `aurore_demo`）。
6. 在 `app/src/main/AndroidManifest.xml` 中用你的 Activity 替代 `DemoActivity`。
7. 在 `app/src/main/res/values/strings.xml` 中设置应用名，在 `app/src/main/res/mipmap-*/ic_launcher.png` 中替换图标。

Activity 被销毁时会调用 `System.exit(0)`：Aurore 把状态保存在静态变量中，否则安卓重新创建 Activity 时 SDL2 会复用这些状态。

## 已知问题

删除资源文件后，libromfs 可能察觉不到，导致编译失败。重新运行 CMake 即可解决：修改一下 `app/jni/CMakeLists.txt`（例如加一个空行），或删除 `app/.cxx`。
