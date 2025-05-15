# Volume.exe

**What and Why:** handle volume controls and a custom shutdown hotkey SPECIFICALLy for an arcade cabinet shelling directly to an emulation frontend.

This little program listens for volume up/down and mute "multimedia" keys, and displays an overlay window showing the current volume level.
It ALSO listens for a special global hotkey (Ctrl+Alt+Y), and when that key combo is detected, it shuts down the computer.

This is for a the very specific use case of an arcade cabinet shelling directly to an emulation frontend. In this case, explorer.exe is NOT around
to pick up the volume keys. This program provides the volume control functionality that is otherwise missing when explorer.exe is not the shell.

The shutdown hotkey is wired to a macro button + some other logic to control power to various components of the cabinet. The hotkey facilitates a SOFT 
shutdown of Windows before power is cut.

In summary this program provides two things:

1) multimedia key handling with on-screen overlay when explorer.exe is not running
2) a global hotkey to shutdown the computer (Ctrl+Alt+Y)


Areas of note:
* search for `0xE9` in the code: this is where we identify which keypress alters the volume. It's currently a multimedia key, but could be changed to a different key.  
* search for `isExplorerShellActive` in the code: this is where we check if explorer owns the windows shell. Having tihs check makes the program difficult to develop/modify, so you may want to null it out if doing any work on the program.
* you could conceivably change the volume activation keys to something else, then you wouldn't need the `isExplorerShellActive`.    


