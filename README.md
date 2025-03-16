# Turbiine

This is an Aroma plugin to turn any Wii U controller into a turbo controller.

> *Turbiine* is the turbocharged shortcut to gaming greatness.

> Blast your way to victory with the ultimate gaming weapon!

> It’s time to level up your game!

<p align="center">
    <a href="https://hb-app.store/wiiu/Turbiine">
        <img src="assets/hbasbadge-wiiu.png" width="335" height="96">
    </a>
</p>


## Usage

- Press the toggle button shortcut (e.g. **TV + ZL** on the Gamepad). A notification will be
  shown.

- Press a button; it will toggle between **normal** and **turbo** mode. A notification
  will be shown.

**Note:** Turbo action is disabled every time you start a game, or return to the Wii U Menu.


## Configuration

To configure the plugin, open the Plugin Config Menu (**L + DOWN + SELECT**) and enter the
**Turbiine** plugin, to access the options:

- **Enabled**: Enables or disables the plugin.

- **Period**: The time interval between turbo actions. Pressing the button is one action,
  releasing the button is one action. For instance, a value of *50 ms* will make the
  button be held down for *50 ms*, then released for *50 ms*, and so on; so a full
  press-release cycle is *50+50 = 100 ms*, giving you 10 button presses per second (bps). The
  default is **16 ms**, which produces roughly 30 bps.

  > **Note:** some games restrict how fast they will register button presses, so you might
  > need to increase the period.
  
  > **Note:** turbo actions can only happen when the game checks the controller's
  > buttons. If a game only reads inputs 30 times per second, the plugin can only generate
  > up to 30 actions per second, or 15 bps.

- **Toggle turbo 1, 2, 3**: Sets the button shortcut for turning turbo *on* or *off*.

  1. Press `A` to focus the button shortcut you want to change.

  2. Hold down the buttons you want to use for the button shortcut, until they are
     registered.

  3. Release all the buttons.

  4. Press either:

       - `A` to confirm the new button shortcut;

       - `B` to cancel;

       - `X`/`1` to reset to the default button shortcut.

  If you leave the button shortcut empty, because you didn't hold any button long enough in
  step *2*, the shortcut will be considered disabled.

- **Reset all turbos...**: Immediately disables all turbo action on all controllers. All
  buttons go back to normal.


## Build instructions

This is a standard Automake package; a Docker build script is also provided.


### Building with Automake

#### Dependencies

- [wut](https://github.com/devkitPro/wut/) - currently, the devkitPro package is too old,
  you will need to compile and install the current wut from the repository.

- [WiiUPluginSystem](https://github.com/wiiu-env/WiiUPluginSystem)

- [libnotifications](https://github.com/wiiu-env/libnotifications)

- [libbuttoncombo](https://github.com/wiiu-env/libbuttoncombo)

If you got a release tarball (`.tar.gz`) you can skip step 0.

0. `./bootstrap`

1. `./configure --host=powerpc-eabi CXXFLAGS='-Os'`

2. `make`

3. (optional) If your Wii U is named `wiiu` in your local network, you can also use these:

   - `make install`: install via ftp, requires `curl` in your system.

   - `make uninstall`: uninstall via ftp, requires `curl` in your system.

   - `make run`: load the plugin without installing, requires `wiiload` package from
     devkitPro.


### Building with Docker

Run the `./docker-build.sh` script.
