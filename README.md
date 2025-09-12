## This is a fork of https://github.com/hrvach/deskhop .   

### Please visit that REPO for all the info and instructions.

The original is brilliant, but not entirely usable for me in the way that I wanted it.  I decided to modify it down to the most barebones state, stripping basically every feature besides hotkey switching, as well as fixing some very specific bugs to myself (like the power key).
I haven't uploaded the code yet, will will do so soon.  Main differences:

- Every hotkey has been removed except the hotkey to switch desktops.
- The remaining hotkey switched to pause, but can easily be changed to F13-F24 or something. This means no more filtering of LCTRL, so no wonky behaviour with that key.
- Web config removed
- Mouse roaming removed
- Screensaver removed
- macos support removed.  I don't need that nonsense.  add it back yourself if you want it.
- Gaming Mode is now the default and only mode.  That means the mouse is always relative and locked to the system its on.
- Made sure commands like power go through to both machines.
- Removed update mode features too because I broke it and couldn't figure out how to get it to work again.  you can just update with the button for bootloader
- Modified the top of the case so you can actually access that button.
- Basically, the only purpose of this device is now to switch computers on a hotkey (pause, and it intercepts it so it never reaches the system), being instantaneous and not disconnecting and giving you that stupid disconnect sound.
- It works for me so I will likely never update this again.
- Added a null mode that drops all outputs to PCs (useful for putting computer to sleep or monitors to sleep)

# you chance the hotkey to switch screens in /src/include/user_config.h

   
