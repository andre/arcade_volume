# Arcade Cabinet Helper

**What and Why:** handle volume controls and a custom shutdown hotkey SPECIFICALLY for an arcade cabinet shelling directly to an emulation frontend.

This little program listens for volume up/down and mute "multimedia" keys, and displays an overlay showing the current volume level.
It ALSO listens for a special global hotkey `(Ctrl+Alt+Y)`, and when that key combo is detected, it shuts down the computer.

This is for the very specific use case of an arcade cabinet shelling directly to an emulation frontend. In this case, `explorer.exe` is not around
to pick up the volume keys. This program provides the volume control functionality that is otherwise missing when `explorer.exe` is not the shell.

The shutdown hotkey is wired to a macro button + some other logic to control power to various components of the cabinet. The hotkey facilitates a SOFT 
shutdown of Windows before power is cut.

**In summary this program provides:**
1) multimedia key handling with on-screen overlay when `explorer.exe` is not running
2) a global hotkey to shutdown the computer `(Ctrl+Alt+Y)`

**Using with Coinops RetroFE**
* Create a `start.bat` file in the Coinops root containing: `start arcade_cabinet_helper.exe`
* Create an `exit.bat` file in the Coinops root containing: `taskkill /im arcade_cabinet_helper.exe`

**Troubleshooting**
* _"It's running, but the volume on-screen-display hasn't changed"_: It only renders its custom volume display if explorer.exe is NOT running. Kill explorer.exe in task manager, and then you'll see the custom volume display.
* _"It works, but the volume display doesn't show when MAME is running"_: MAME runs full-screen and it's complicated to get an OSD to render over a full-screen app. The volume control still works, and you'll still get the audio feedback of volume change.


**Areas to note:**
* search for `0xE9` in the code: this is where we identify which keypresses alter the volume. It's currently multimedia keys, but could be changed to anything.  
* search for `isExplorerShellActive` in the code: this is where we check if explorer is the windows shell. Having tihs check makes the program difficult to develop/modify, so you may want to stub the return value to do any work on this. Just note though, if explorer.exe IS running and the program looks for the standard multimedia volume keys, BOTH this program AND explorer will capture the volume change events -- resulting in two on-screen volume displays, AND the volume changing faster with each keypress than if just one or the other were running.
* If your cabinet is set up so your volume buttons produce some other key stroke (it could be a chorded keystroke, like cnt-alt-e or something) you coulde change the keys this program is looking for to change the volume -- that way, you probably won't need the `isExplorerShellActive` check. Although in that case, your volume will only work when this program is running. Tradeoffs.

**Compatiblity**
This is only for Windows. 

**Compilation** 
* See `tasks.json` for build and debug compilation with vscode.
* if you're new to vscode (or you're me looking at this a year from now), you have to start vscode as follows: 1) install a bunch of crap using Visual studio installer; 2) launch the "x64 native tools command prompt (windows key, type x64); 3) once the command prompt is up, type `code`, and vscode will open with all the paths needed to compile for x64.
* I have checked in the `resources.res`, which just contains a sound file. If you change the sound, you need to remake the resource, which is currently not handled in vscode's `tasks.json` to rebuild from the command line: `rc resource.rc`. This will regenerate `resources.res`, which in turn is referenced in the compile/link commands in `tasks.json`.




