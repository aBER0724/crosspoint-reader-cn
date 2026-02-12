# CrossPoint Reader CJK

**[English](./README.md)** | [中文](./README-ZH.md) | [日本語](./README-JA.md)

> A CJK adapted version of the **Xteink X4** e-ink reader firmware based on [daveallie/crosspoint-reader](https://github.com/daveallie/crosspoint-reader).

This project adapts the original CrossPoint Reader for CJK support, featuring a multi-language interface and CJK font rendering.

![](./docs/images/cover.jpg)

## ✨ New CJK Features

### 🌏 Multi-language Interface Support (I18n)

- **Complete Localization**: Supports Chinese, English, and Japanese interface languages.
- Switch interface language anytime in Settings.
- All menus, prompts, and settings are fully localized.
- Dynamic translation system based on string IDs.

### 📝 CJK Font System

- **External Font Support**:
  - **Reading Font**: Used for book content (selectable size and font family).
  - **UI Font**: Used for menus, titles, and the interface.
  - Font sharing option: Use the reading font as the UI font to save memory.
- LRU cache optimization to improve CJK rendering performance.
- Built-in ASCII character fallback mechanism to reduce memory usage.
- Example Fonts:
  - [Source Han Sans CN](fonts/SourceHanSansCN-Bold_20_20x20.bin) (UI Font)
  - [King Hwa Old Song](fonts/KingHwaOldSong_38_33x39.bin) (Reader Font)

### 🎨 Themes & Display

- **Lyra Theme**: A modern UI theme with rounded selection highlights, scrollbar pagination, and refined layout metrics — fully adapted for CJK pages.
- **Dark/Light Mode Switching**: Applies to both reader and UI.
- Smooth theme switching without full refresh.

### 📖 Reading Layout

- **First Line Indent**: Toggle paragraph indentation via CSS `text-indent` for improved readability.
- Indent width calculated based on actual CJK character width.
- **Streaming CSS Parser**: Handles large stylesheets without running out of memory.
- **CJK Word Spacing Fix**: Removes spurious gaps between adjacent CJK characters.

## 📦 Feature List

- [x] EPUB Parsing & Rendering (EPUB 2 and EPUB 3)
- [x] EPUB Image Alt Text Display
- [x] TXT Plain Text Reading Support
- [x] Reading Progress Saving
- [x] File Browser (Supports nested folders)
- [x] Custom Sleep Screen (Supports cover display)
- [x] KOReader Reading Progress Sync
- [x] WiFi File Upload
- [x] WiFi OTA Firmware Update
- [x] WiFi Credential Management (Scan, Save, Delete via Web UI)
- [x] AP Mode Improvements (Captive Portal Support)
- [x] Dark/Light Mode Switching
- [x] First Line Indent for Paragraphs
- [x] Multi-language Hyphenation Support
- [x] Font, Layout, Display Style Customization
  - [x] External Font System (Reading + UI Fonts)
- [x] Screen Rotation (Independent UI/Reader orientation settings)
  - [x] Reading Interface Rotation (Portrait, Landscape CW/CCW, Inverted)
  - [x] UI Interface Rotation (Portrait, Inverted)
- [x] Calibre Wireless Connection & Web Library Integration
- [x] Cover Image Display
- [x] Lyra Theme (with full CJK page adaptation)
- [x] KOReader Reading Progress Sync
- [x] Streaming CSS Parser (prevents OOM on large EPUB stylesheets)

For detailed operation instructions, please refer to the [User Guide](./USER_GUIDE.md).

## 📥 Installation

### Web Flashing (Recommended)

1. Connect the Xteink X4 to your computer via USB-C.
2. Visit https://xteink-flasher-cjk.vercel.app/ and click **"Flash CrossPoint CJK firmware"** to flash the latest CJK firmware directly.

> **Tip**: To restore official firmware or the original CrossPoint firmware, use the same page. To switch the boot partition, visit https://xteink-flasher-cjk.vercel.app/debug.

### Manual Compilation

#### Requirements

* **PlatformIO Core** (`pio`)
* Python 3.8+
* USB-C Cable
* Xteink X4

#### Get Code

```bash
git clone --recursive https://github.com/aBER0724/crosspoint-reader-cjk

# If submodules are missing after cloning
git submodule update --init --recursive
```

#### Build & Flash

Run the following command after connecting the device:

```bash
pio run --target upload
```

## 🔠 Fonts

### Font Generation

- `tools/generate_cjk_ui_font.py`

- [DotInk](https://apps.apple.com/us/app/dotink-eink-assistant/id6754073002) (Recommended: Use DotInk to generate reading fonts for a better preview of how the font looks on the device).

### Font Configuration

1. Create a `fonts/` folder in the root directory of the SD card.
2. Place font files in `.bin` format into the folder.
3. Select "Reader/Reader Font" or "Display/UI Font" in settings.

**Font File Naming Format**: `FontName_size_WxH.bin`

Examples:
- `SourceHanSansCN-Medium_20_20x20.bin` (UI: 20pt, 20x20)
- `KingHwaOldSong_38_33x39.bin` (Reading: 38pt, 33x39)

**Font Description**:
- **Reading Font**: Used for book content text.
- **UI Font**: Used for menus, titles, and interface elements.

> Due to memory constraints, the built-in font uses a very small Chinese character set, containing only the characters required for the UI.
> It is recommended to store more complete UI and reading fonts on the SD card for a better experience.
> If generating fonts is inconvenient, you can download the example fonts provided above.

## ℹ️ FAQ

1. For chapters with many pages, indexing may take longer on first open. This has been significantly improved with the streaming CSS parser, but very large chapters may still require a few seconds.
    > If the device remains stuck indexing after a reboot and fails to complete for a long time, preventing you from returning to other pages, please re-flash the firmware.
2. If stuck on a specific interface, try restarting the device.
3. The ESP32-C3 has very limited memory. Using large CJK font files for both UI and reading fonts simultaneously may cause out-of-memory crashes. It is recommended to keep UI fonts at 20pt or below.
4. When opening the home screen for the first time after adding new books, the device will generate cover thumbnails. A "Loading" popup may appear for a few seconds — this is normal, not a freeze.

## 📜 Credits

- [CrossPoint Reader](https://github.com/daveallie/crosspoint-reader) - Original Project
- [open-x4-sdk](https://github.com/daveallie/open-x4-sdk) - Xteink X4 Development SDK

---

**Disclaimer**: This project is not affiliated with Xteink or the X4 hardware manufacturer; it is purely a community project.
