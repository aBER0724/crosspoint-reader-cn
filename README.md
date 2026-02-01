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

- **Dark/Light Mode Switching**: Applies to both the reader and UI.
- Smooth theme switching without full refresh.

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
- [x] Dark/Light Mode Switching
- [x] Multi-language Hyphenation Support
- [x] Font, Layout, Display Style Customization
  - [x] External Font System (Reading + UI Fonts)
- [ ] Screen Rotation
  - [x] Reading Interface Rotation
  - [ ] UI Interface Rotation
- [x] Calibre Wireless Connection & Web Library Integration
- [x] Cover Image Display

For detailed operation instructions, please refer to the [User Guide](./USER_GUIDE.md).

## 📥 Installation

### Web Flashing (Recommended)

1. Connect the Xteink X4 to your computer via USB-C.
2. Visit the [Releases page](https://github.com/aBER0724/crosspoint-reader-cjk/releases) to download the latest firmware.
3. Visit https://xteink.dve.al/ and use the "OTA fast flash controls" to flash the firmware.

> **Tip**: To restore official firmware, visit the same URL or switch the boot partition at https://xteink.dve.al/debug.

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

1. If a chapter has many pages, the number of unindexed characters may increase, leading to longer indexing times.
    > If the device remains stuck indexing after a reboot and fails to complete for a long time, preventing you from returning to other pages, please re-flash the firmware.
2. If stuck on a specific interface, try restarting the device.

## 📜 Credits

- [CrossPoint Reader](https://github.com/daveallie/crosspoint-reader) - Original Project
- [open-x4-sdk](https://github.com/daveallie/open-x4-sdk) - Xteink X4 Development SDK

---

**Disclaimer**: This project is not affiliated with Xteink or the X4 hardware manufacturer; it is purely a community project.
