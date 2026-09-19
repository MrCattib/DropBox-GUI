# DropBox Client (Unofficial)

A lightweight, self-contained desktop GUI for Dropbox.

This is **not** an official Dropbox application and is not affiliated with Dropbox, Inc.

---

### Why this exists

The official Dropbox desktop client is basically just a tray icon + a magic folder.  
You drop files in and hope they sync.

This app gives you an actual window with the full Dropbox web interface, proper file dialogs, working Google/Apple logins, and tray support so you don’t have to reload everything every time.

---

### Features

- Real window instead of tray-only
- Persistent login (cookies & session saved)
- System tray support (close = hide, not quit)
- Native file open/save/folder dialogs
- Working OAuth popups (Google, Apple, etc.)
- Per-user data isolation (`~/.dropbox-client`)
- Self-contained (ships with its own libraries)
