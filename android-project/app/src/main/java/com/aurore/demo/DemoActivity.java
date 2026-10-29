package com.aurore.demo;
import android.os.Bundle;

import org.libsdl.app.AuroreHandler;
import org.libsdl.app.PlatformUtils;
import org.libsdl.app.SDLActivity;

public class DemoActivity extends SDLActivity
{


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Currently we use handler to receive brightness changes from Aurore
        PlatformUtils.auroreHandler = new AuroreHandler();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        // Android does not recommend using exit(0) directly,
        // but Aurore heavily uses static variables,
        // which can cause some problems when reloading the program.

        // In SDL3, we can use SDL_HINT_ANDROID_ALLOW_RECREATE_ACTIVITY to control the behavior

        // In SDL2, Force exit of the app.
        System.exit(0);
    }

    @Override
    protected String[] getLibraries() {
        // Load SDL (SDL2 or SDL3, app/build.gradle's auroreSdl) and the
        // Aurore demo app
        return new String[] {
                BuildConfig.SDL_LIBRARY,
                "aurore_demo"
        };
    }

}