# Turbiine

> *Turbiine* is the turbocharged shortcut to gaming greatness.

> Blast your way to victory with the ultimate gaming weapon!

> It’s time to level up your game!

This is an Aroma plugin to turn any Wii U controller into a turbo controller.


## Usage

  - Press the shortcut button combo;

  - Press a button; it will toggle between normal and turbo mode.

You will see a notification on screen showing the normal/turbo state changed.

Only the controller where you pressed the shortcut will be affected.

**Note:** Turbo action is reset every time you start a game, or return to the Wii U Menu.


## Configuration

To configure the plugin, open the Plugin Config Menu (**L + ↓ + SELECT**) and enter the
**Turbiine** plugin, to access the options:

- **Enabled**: Enables or disables the plugin.

- **Period**: Configures how long each turbo press lasts, in read samples. Higher values
  mean slower turbo action.

- **Reset all turbos...**: Immediately disables all turbo action on all controllers.

- **Toggle 1, 2, 3, 4**: Sets the combo for turning turbo on or off.

   1. Press `A` to focus the combo you want to change.
   2. Hold the buttons you want for the combo, until they are recorded.
   3. Release all the buttons. Press `A` to confirm the new combo, `B` to cancel, `X`/`1`
      to reset to the default combo.

    If you leave the combo empty, because you didn't hold any button long enough in step
    **2**, the combo will be considered disabled.


## Build instructions

This is a standard Automake package; a Docker build script is also provided.


### Building with Automake

If you got a release tarball (`.tar.gz`) you can skip step 0.

0. `./bootstrap`

1. `./configure --host=powerpc-eabi CXXFLAGS='-Os'`

2. `make`


### Building with Docker

Run the `./docker-build.sh` script.

