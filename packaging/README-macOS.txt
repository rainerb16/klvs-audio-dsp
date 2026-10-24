Rainr Audio {PLUGIN} {VERSION} for macOS (test build)

This build isn't signed by Apple yet. Install it like this:

1. Open Terminal (Applications > Utilities) and paste:

   cd ~/Downloads/Rainr-{PLUGIN}-{VERSION}-macOS
   mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
   cp -R {PLUGIN}.vst3 ~/Library/Audio/Plug-Ins/VST3/
   cp -R {PLUGIN}.component ~/Library/Audio/Plug-Ins/Components/
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/{PLUGIN}.vst3 ~/Library/Audio/Plug-Ins/Components/{PLUGIN}.component
   killall -9 AudioComponentRegistrar 2>/dev/null
   ls -d ~/Library/Audio/Plug-Ins/VST3/{PLUGIN}.vst3 ~/Library/Audio/Plug-Ins/Components/{PLUGIN}.component

   The last line should print the two installed paths. "No such file or directory" means the
   cd line didn't find this folder: type "cd " in Terminal, drag this folder onto the window,
   press Return, then paste the other lines again.

   (The xattr line tells macOS these two files are OK to load. It changes nothing else.)

   Note: "~/Library" is the Library inside your home folder (hidden in Finder), not
   Macintosh HD > Library. To look: Finder > Shift-Cmd-G > ~/Library/Audio/Plug-Ins/VST3

2. In Ableton Live: Settings > Plug-Ins > turn on "Use VST3 Plug-in System Folders" > Rescan.
   Find it under Plug-Ins > Rainr Audio > {PLUGIN}. Use the VST3 version.

Updating: quit Live and delete the old {PLUGIN}.vst3 and {PLUGIN}.component from those two folders
first, then install as above.

Full instructions and troubleshooting:
https://github.com/rainerb16/rainr-audio-dsp/blob/main/docs/INSTALL.md
