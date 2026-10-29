// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace aurore
{

enum Sound
{
    SOUND_NONE = 0, // no sound

    SOUND_FOCUS_CHANGE, // played when the focus changes
    SOUND_FOCUS_ERROR, // played when the user wants to go somewhere impossible (while the highlight wiggles)
    SOUND_CLICK, // played when the click action runs
    SOUND_BACK, // played when back action runs
    SOUND_FOCUS_SIDEBAR, // played when the focus changes to a sidebar item
    SOUND_CLICK_ERROR, // played when the user clicks a disabled button / a view focused with no click action
    SOUND_HONK, // honk
    SOUND_CLICK_SIDEBAR, // played when a sidebar item is clicked
    SOUND_TOUCH_UNFOCUS, // played when touch focus has been interrupted
    SOUND_TOUCH, // played when touch doesn't require it's own click sound
    SOUND_SLIDER_TICK,
    SOUND_SLIDER_RELEASE,

    _SOUND_MAX, // not an actual sound, just used to count of many sounds there are
};

// Platform agnostic Audio player
// Each platform's AudioPlayer is responsible for managing the enum Sound -> internal representation map
class AudioPlayer
{
  public:
    virtual ~AudioPlayer() {};

    /**
     * Preemptively loads the given sound so that it's ready to be played
     * when needed.
     *
     * Returns a boolean indicating if the sound has been loaded or not.
     */
    virtual bool load(enum Sound sound) = 0;

    /**
     * Plays the given sound.
     *
     * The AudioPlayer should not assume that the sound has been
     * loaded already, and must load it if needed.
     *
     * Returns a boolean indicating if the sound has been played or not.
     */
    virtual bool play(enum Sound sound, float pitch = 1) = 0;
};

// An AudioPlayer that does nothing
class NullAudioPlayer : public AudioPlayer
{
  public:
    bool load(enum Sound sound) override
    {
        return false;
    }

    bool play(enum Sound sound, float pitch) override
    {
        return false;
    }
};

} // namespace aurore
