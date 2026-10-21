# Moonlight for Landfall

This is Moonlight 6.2.0 ([moonlight-stream/moonlight-qt](https://github.com/moonlight-stream/moonlight-qt), GPL-3.0)
with small changes for [Landfall](https://github.com/ToolsForCavemen), which bundles it.

## Changes

- **Files dropped on the stream window** (from any file manager, on Linux X11/Wayland, Windows and macOS) are
  reported on stderr as one line, `LANDFALL-SOLTAR <x> <y> <path> [<path>...]`. `x` and `y` are thousandths of the
  remote screen, using the same video region as the absolute mouse mode. Each path is percent-encoded.
  Landfall reads that line, sends the files to the remote PC and drops them at that same point.
  The position comes from the SDL3 library underneath sdl2-compat. If it isn't there, the pointer position is used.
  Code: `app/streaming/landfalldrop.cpp`, called from the event loop in `app/streaming/session.cpp`.
- **System shortcuts in desktop (absolute mouse) mode:** with "capture system keys" on, the keyboard is grabbed whenever
  the stream window has focus, not only while the mouse is captured (in absolute mouse mode it almost never is, so
  Alt+Tab and the Windows key stayed on the local desktop). With `--capture-system-keys fullscreen`, "fullscreen" also
  counts a window that covers its display (Hyprland fullscreen, which SDL doesn't always flag). The grab is updated on
  focus changes and resizes. Code: `updateKeyboardGrabState()` in `app/streaming/input/input.cpp`.
- **Keys held when the window gets focus are not sent** (neither the press nor the release): arriving at the stream
  with a desktop shortcut such as Super+2 used to send a lone Windows-key tap and open the Start menu. Code:
  `notifyFocusGained()` in `input.cpp`, `handleKeyEvent()` in `keyboard.cpp`.
- **Key rules (who gets each shortcut):** Landfall writes the rules of the current connection to the file named by
  `$LANDFALL_TECLAS`, one per line: `aca Super+Space` (stays on the local computer) or `alla Alt+Tab` (goes to the host,
  the default). Key names are SDL scancode names. When an `aca` combination arrives while the keyboard is grabbed, it
  isn't sent: modifiers already sent to the host are released there, the keyboard grab is dropped for 0.7 s, and
  `LANDFALL-TECLA <combination>` is written on stderr. Landfall then presses that combination on the local computer, so
  the local desktop runs its own shortcut. The file is re-read on focus gain. Code: `app/streaming/landfallkeys.cpp`,
  `landfallKeepLocal()` in `keyboard.cpp`.
- **X11 pointer without focus is ignored:** under XWayland the stream window on a hidden workspace stays on top of the
  X11 stack and received the pointer moving over other X11 windows of the visible workspace, which moved (and could
  click) the remote pointer. With the x11 video driver, motion, button presses and wheel are dropped while the stream
  window has no keyboard focus (button releases still go through). Code: `landfallPointerNotOurs()` in `session.cpp`.
- **macOS:** the app bundle is ad-hoc signed when no signing identity is set (`scripts/generate-dmg.sh`).
- **CI:** `.github/workflows/landfall.yml` builds the three packages and publishes them on tags `landfall-*`.

Everything else is upstream Moonlight.
