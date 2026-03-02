# 💎 CrystalVST
**Universal Granular Plugin (Mac Intel & Apple Silicon)**

CrystalVST is a deep granular synthesizer and processor with a 10s memory buffer, rhythmic morphing, and a minimal "sober" interface.

## 🎧 Installation (macOS)

1.  Open the [Releases/](Releases/) folder in this repository.
2.  **`CrystalVST.vst3`**: Copy to `/Library/Audio/Plug-Ins/VST3/`
3.  **`CrystalVST.component`**: Copy to `/Library/Audio/Plug-Ins/Components/`
4.  **`CrystalVST.app`**: Standalone version (runs without a DAW).

## 🛡️ macOS Security Fix
If you see a message saying "macOS cannot verify the developer":
1.  Try to open the plugin/app once.
2.  Open **System Settings** > **Privacy & Security**.
3.  Scroll down to the **Security** section.
4.  Click **"Open Anyway"** for CrystalVST.

Alternatively, run this in Terminal:
`sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/CrystalVST.vst3`

## ✨ Features
- **Universal Binary**: Native support for both Intel and Apple Silicon Macs.
- **Deep Memory**: 10s circular buffer with 8s random seek range.
- **Duration Morphing**: Grains can double, halve, or change to triplets/quintuplets (1/3, 3x, 1/5, 5x) dynamically.
- **Rhythmic Synchronization**: Automatically syncs to host BPM.
