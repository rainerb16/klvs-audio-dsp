# Installing Rainr Audio test builds

These are **test builds** for friends. They aren't signed by Apple or Microsoft yet, so your
computer will be cautious about them the first time. This page shows how to install them safely.

What you need:
- **Mac:** macOS 11 or newer, Intel or Apple Silicon. Ableton Live 10.1 or newer.
- **Windows:** Windows 10 or 11, 64-bit. Ableton Live 10.1 or newer.

The examples use WRM1 version 0.1.0; swap in the version you downloaded.

---

## 1. Download

**Friends:** open the [Releases page](https://github.com/rainerb16/rainr-audio-dsp/releases), pick
the newest WRM1 release and download `Rainr-WRM1-<version>-macOS.zip` or
`Rainr-WRM1-<version>-Windows.zip`. No GitHub account needed.

**Rainer (builds between releases):** repo → **Actions** → the newest green run **on the `main`
branch** → **Artifacts** at the bottom. This needs a GitHub login, and the download is a zip *inside* a zip: unzip twice.

---

## 2. Mac

### Install

1. Double-click the zip in Downloads. You get a folder `Rainr-WRM1-0.1.0-macOS` with
   `WRM1.vst3` (VST3), `WRM1.component` (Audio Unit) and a `README.txt`.
2. Open **Terminal** (Applications → Utilities), paste this whole block and press Return:

   ```
   cd ~/Downloads/Rainr-WRM1-0.1.0-macOS
   mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
   cp -R WRM1.vst3 ~/Library/Audio/Plug-Ins/VST3/
   cp -R WRM1.component ~/Library/Audio/Plug-Ins/Components/
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/WRM1.vst3 ~/Library/Audio/Plug-Ins/Components/WRM1.component
   killall -9 AudioComponentRegistrar 2>/dev/null
   ls -d ~/Library/Audio/Plug-Ins/VST3/WRM1.vst3 ~/Library/Audio/Plug-Ins/Components/WRM1.component
   ```

   It copies the plugin into your own plugin folders (no admin password needed) and tells macOS
   these two files are OK to load. Nothing else on your Mac changes.

3. **Check it worked:** the last line prints the two installed paths. If it says
   "No such file or directory" instead, nothing was installed: usually the `cd` line didn't find
   the folder (see the tip below).

**Good to know**

- **`~/Library` is the Library inside your home folder, not Macintosh HD → Library.** Your Mac
  has two Library folders. The guide uses the one in your home folder, which Finder hides. To see
  the installed plugin: in Finder press **Shift-Cmd-G** and enter `~/Library/Audio/Plug-Ins/VST3`.
  Don't also put WRM1 in Macintosh HD → Library → Audio → Plug-Ins: two copies can confuse Live.
- **The plugin is copied, not moved,** so the downloaded folder stays in Downloads. You can delete
  it (and the zip) afterwards.
- **Folder somewhere else?** If your download isn't directly in Downloads (or unzipped into a
  folder with a different name), type `cd ` (with a space) in Terminal, drag the
  `Rainr-WRM1-…-macOS` folder from Finder onto the Terminal window, press Return, then paste the
  block again without its first line.

### Why the Terminal step, and is it safe?

Everything downloaded from the internet gets a "quarantine" tag. For an app, macOS then offers
**Open Anyway** in System Settings → Privacy & Security. Plugins are loaded by Ableton, not opened
by you, so that button often never appears and Live just reports that the plugin failed to load
or "is damaged". The `xattr` line removes the quarantine tag from **these two files only**.

- It does not switch off Gatekeeper or any other protection.
- Only do it for files you got from someone you trust, from this repo's Releases page.
- Never run `sudo spctl --master-disable` or similar "disable Gatekeeper" advice from the internet.

### Prefer Finder? (manual way)

1. In Finder: **Go → Go to Folder…** (Shift-Cmd-G), enter `~/Library/Audio/Plug-Ins/VST3` (type
   it exactly, including `~`) and drag `WRM1.vst3` in. Do the same with
   `~/Library/Audio/Plug-Ins/Components` and `WRM1.component`. If a folder doesn't exist yet,
   create it with that exact name.
2. You still need the `xattr` line from the block above, run in Terminal.

---

## 3. Windows

1. **Before unzipping:** right-click the zip → **Properties** → tick **Unblock** (bottom of the
   General tab) → OK. This removes the "downloaded from the internet" mark from everything inside.
   If there's no Unblock box, Windows didn't mark it and you can skip this.
2. Extract the zip. You get a folder `Rainr-WRM1-0.1.0-Windows` containing a `WRM1.vst3` folder
   and a `README.txt`.
3. Copy the whole `WRM1.vst3` folder to `C:\Program Files\Common Files\VST3`
   (create the `VST3` folder if it doesn't exist). Windows asks for administrator permission:
   that's normal for this folder.

You won't see a SmartScreen "Windows protected your PC" warning: that appears for programs you
run, and a plugin is loaded by Ableton instead. If your antivirus questions the file, it's because
the build is new and unsigned; only allow it if it came from this repo's Releases page.

---

## 4. Ableton Live

1. Open **Settings** (Mac: Live → Settings…, Windows: Options → Preferences) → **Plug-Ins**.
2. Turn on **Use VST3 Plug-in System Folders**. On a Mac, to use the Audio Unit version, also
   turn on **Use Audio Units** (Live 12 calls it **Use Audio Units v2**).
3. Click **Rescan**. If WRM1 doesn't appear, hold **Option** (Mac) / **Alt** (Windows) while
   clicking Rescan for a full rescan.
4. In the browser: **Plug-Ins** → **Rainr Audio** → **WRM1**. Drag it onto your master track.

**VST3 or AU?** Use the **VST3** version. It's the same plugin on Mac and Windows, so a Live
set you send a friend on the other platform opens with WRM1 intact. Both versions sound
identical.

---

## 5. Updating and removing

- **Update:** quit Live first, remove the old version (below), then install the new one exactly as
  above and Rescan. Copying over a plugin that's still in place, or still loaded in Live, can leave
  a mix of old and new files and make macOS reject it. Your saved sets keep their WRM1 settings:
  parameter names never change between versions.
- **Remove (Mac):** delete `WRM1.vst3` from `~/Library/Audio/Plug-Ins/VST3` and `WRM1.component`
  from `~/Library/Audio/Plug-Ins/Components`.
- **Remove (Windows):** delete `C:\Program Files\Common Files\VST3\WRM1.vst3`.

---

## 6. Something wrong?

| What you see | Try |
|---|---|
| Mac: Live says WRM1 failed to load / is damaged | Run the `xattr` line from step 2 again, then Rescan |
| Mac: the plugin isn't in Macintosh HD → Library → Audio → Plug-Ins | That's expected: it's in the Library in your home folder (see "Good to know") |
| Mac: you also dragged WRM1 into Macintosh HD → Library → Audio → Plug-Ins | Remove that extra copy so Live only sees one: `rm -rf /Library/Audio/Plug-Ins/VST3/WRM1.vst3 /Library/Audio/Plug-Ins/Components/WRM1.component` |
| WRM1 isn't in the Plug-Ins list | Check the plugin is in the folder above, the Live setting is on, then Option/Alt-click Rescan |
| Only the VST3 or only the AU shows up | Turn on the other format in Live's Plug-Ins settings |
| Clicks, crashes, or it sounds wrong | Tell Rainer (below) |

When you report a problem, include: WRM1 version (top right of the plugin window), Mac or
Windows and its version, Intel or Apple Silicon on a Mac, Ableton Live version, and what you did
just before it happened. A screenshot or a short bounce helps a lot.

**Needs verification:** the exact names of Live's Plug-Ins settings differ slightly between Live
versions. If the names above don't match yours, look for the VST3 and Audio Units switches on the
same page.
