/*
    Copyright 2023 xfangfang

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <SDL2/SDL.h>
#include <arpa/inet.h>
#include <jni.h>
#include <net/if.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/android/android_platform.hpp>

namespace brls
{

// A Java exception left pending makes the next JNI call abort: logged and
// cleared. True when there was one.
static bool clearException(JNIEnv* env)
{
    if (!env->ExceptionCheck())
        return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

// PlatformUtils and its static method `name`; false, nothing to delete,
// when either is not there
static bool findUtilsMethod(JNIEnv* env, const char* name, const char* signature, jclass* utilsClass,
    jmethodID* method)
{
    *utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (clearException(env) || *utilsClass == nullptr)
        return false;
    *method = env->GetStaticMethodID(*utilsClass, name, signature);
    if (clearException(env) || *method == nullptr)
    {
        env->DeleteLocalRef(*utilsClass);
        return false;
    }
    return true;
}

// `text`, UTF-8, as a Java string: NewStringUTF reads "modified UTF-8",
// which a character outside the BMP (an emoji) is not. Null when it cannot.
static jstring javaString(JNIEnv* env, const std::string& text)
{
    jstring result   = nullptr;
    jbyteArray bytes = env->NewByteArray((jsize)text.size());
    if (clearException(env) || bytes == nullptr)
        return nullptr;
    env->SetByteArrayRegion(bytes, 0, (jsize)text.size(), reinterpret_cast<const jbyte*>(text.data()));
    jclass stringClass = env->FindClass("java/lang/String");
    if (!clearException(env) && stringClass != nullptr)
    {
        jmethodID init  = env->GetMethodID(stringClass, "<init>", "([BLjava/lang/String;)V");
        jstring charset = nullptr;
        if (!clearException(env) && init != nullptr)
            charset = env->NewStringUTF("UTF-8");
        if (!clearException(env) && charset != nullptr)
        {
            result = (jstring)env->NewObject(stringClass, init, bytes, charset);
            if (clearException(env))
                result = nullptr;
        }
        if (charset != nullptr)
            env->DeleteLocalRef(charset);
        env->DeleteLocalRef(stringClass);
    }
    env->DeleteLocalRef(bytes);
    return result;
}

bool static getPlatformBool(const std::string& method, bool defaultValue = false)
{
    auto env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jclass utilsClass;
    jmethodID jmethod;
    if (!findUtilsMethod(env, method.c_str(), "()Z", &utilsClass, &jmethod))
        return defaultValue;
    jboolean value = env->CallStaticBooleanMethod(utilsClass, jmethod);
    if (clearException(env))
        value = defaultValue;
    env->DeleteLocalRef(utilsClass);
    return value;
}

int static getPlatformInt(const std::string& method, int defaultValue = 100)
{
    auto env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jclass utilsClass;
    jmethodID jmethod;
    if (!findUtilsMethod(env, method.c_str(), "()I", &utilsClass, &jmethod))
        return defaultValue;
    jint value = env->CallStaticIntMethod(utilsClass, jmethod);
    if (clearException(env))
        value = defaultValue;
    env->DeleteLocalRef(utilsClass);
    return value;
}

bool AndroidPlatform::canShowBatteryLevel()
{
    return getPlatformBool("isBatterySupported");
}

int AndroidPlatform::getBatteryLevel()
{
    return getPlatformInt("getBatteryLevel");
}

bool AndroidPlatform::isBatteryCharging()
{
    return getPlatformBool("isBatteryCharging");
}

bool AndroidPlatform::canShowWirelessLevel()
{
    return getPlatformBool("isWifiSupported");
}

bool AndroidPlatform::hasWirelessConnection()
{
    return getPlatformBool("isWifiConnected");
}

int AndroidPlatform::getWirelessLevel()
{
    int level = getPlatformInt("getWifiSignalStrength");
    if (level >= -60)
    {
        return 3;
    }
    else if (level >= -70)
    {
        return 2;
    }
    else if (level >= -80)
    {
        return 1;
    }
    return 0;
}

bool AndroidPlatform::hasEthernetConnection()
{
    return getPlatformBool("isEthernetConnected");
}

std::string AndroidPlatform::getIpAddress()
{
    std::string ipaddr = "-";
    int sock;
    struct ifconf conf;
    char data[4096];
    struct ifreq* ifr;
    struct sockaddr_in* sock_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return "-";
    }

    conf.ifc_len = sizeof(data);
    conf.ifc_buf = (caddr_t)data;
    if (ioctl(sock, SIOCGIFCONF, &conf) < 0)
    {
        close(sock);
        return "-";
    }

    ifr = (struct ifreq*)data;
    while ((char*)ifr < data + conf.ifc_len)
    {
        if (ifr->ifr_addr.sa_family == AF_INET && memcmp(ifr->ifr_name, "lo", 2) != 0)
        {
            sock_addr          = (struct sockaddr_in*)&ifr->ifr_addr;
            const char* result = inet_ntoa(sock_addr->sin_addr);
            if (result)
            {
                ipaddr = std::string { result };
                break;
            }
        }
        ifr = (struct ifreq*)((char*)ifr + sizeof(*ifr));
    }
    close(sock);

    return ipaddr;
}

std::string AndroidPlatform::getDnsServer()
{
    return "-";
}

void AndroidPlatform::openBrowser(std::string url)
{
    if (!isBrowserUrl(url))
    {
        Logger::warning("open url: refused, not an http(s) link");
        return;
    }
    auto env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jclass utilsClass;
    jmethodID openBrowserMethod;
    if (!findUtilsMethod(env, "openBrowser", "(Ljava/lang/String;)V", &utilsClass, &openBrowserMethod))
        return;
    jstring jurl = javaString(env, url);
    if (jurl != nullptr)
    {
        env->CallStaticVoidMethod(utilsClass, openBrowserMethod, jurl);
        clearException(env);
        env->DeleteLocalRef(jurl);
    }
    env->DeleteLocalRef(utilsClass);
}

float AndroidPlatform::getBacklightBrightness()
{
    auto env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jclass utilsClass;
    jmethodID jmethod;
    if (!findUtilsMethod(env, "getAppScreenBrightness", "(Landroid/app/Activity;)F", &utilsClass, &jmethod))
        return 0.0f;
    auto activity = (jobject)SDL_AndroidGetActivity();
    if (activity == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return 0.0f;
    }
    jfloat value = env->CallStaticFloatMethod(utilsClass, jmethod, activity);
    if (clearException(env))
        value = 0.0f;
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(utilsClass);
    return value;
}

void AndroidPlatform::setBacklightBrightness(float brightness)
{
    auto env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jclass utilsClass;
    jmethodID method;
    if (!findUtilsMethod(env, "setAppScreenBrightness", "(Landroid/app/Activity;F)V", &utilsClass, &method))
        return;
    auto activity = (jobject)SDL_AndroidGetActivity();
    if (activity == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return;
    }
    env->CallStaticVoidMethod(utilsClass, method, activity, brightness);
    clearException(env);
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(utilsClass);
}

bool AndroidPlatform::canSetBacklightBrightness()
{
    return true;
}

} // namespace brls
