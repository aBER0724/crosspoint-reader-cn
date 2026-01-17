# CrossPoint Reader CJK

**[English](./README.md)** | [中文](./README-ZH.md) | [日本語](./README-JA.md)

> A CJK-adapted fork of [daveallie/crosspoint-reader](https://github.com/daveallie/crosspoint-reader) firmware for the **Xteink X4** e-paper reader.

This project extends the original CrossPoint Reader with Chinese/Japanese localization support and CJK font rendering capabilities.

![](./docs/images/cover.jpg)

## ✨ New Features in CJK Edition

### 🌏 Multi-language UI Support (I18n)
- Supports **Chinese**, **English**, and **Japanese** interface languages
- Switch languages anytime in Settings
- All menus, prompts, and settings are fully localized

### 📝 CJK Font Support
- **External font loading**: Load custom CJK fonts from SD card `/fonts/` directory
- Supports Xteink standard `.bin` font format
- Built-in LRU cache optimization for improved CJK rendering performance
- Sample fonts included:
  - `SourceHanSansCN-Medium_20_20x20.bin` (Source Han Sans)
  - `KingHwaOldSong_38_33x39.bin` (KingHwa Old Song)

## 📦 Features

Inherits all features from the original CrossPoint Reader:

- [x] EPUB parsing and rendering (EPUB 2 and EPUB 3)
- [x] EPUB image alt text display
- [x] TXT plain text reader support
- [x] Reading progress saved
- [x] File browser (supports nested folders)
- [x] Custom sleep screen (supports cover display)
- [x] WiFi file upload
- [x] WiFi OTA firmware updates
- [x] Configurable font, layout, and display options
- [x] Screen rotation
- [x] Calibre wireless connection and web library integration
- [x] Cover image display

See the [User Guide](./USER_GUIDE.md) for detailed instructions.

## 📥 Installation

### Web Flash (Recommended)

1. Connect your Xteink X4 to your computer via USB-C
2. Go to https://xteink.dve.al/ and click "Flash CrossPoint firmware"

> **Tip**: To restore official firmware, visit the same URL, or switch boot partitions at https://xteink.dve.al/debug

### Manual Build

See the [Development](#development) section below.

## 🔧 Development

### Prerequisites

* **PlatformIO Core** (`pio`) or **VS Code + PlatformIO IDE**
* Python 3.8+
* USB-C cable
* Xteink X4 device

### Getting the Code

```bash
git clone --recursive https://github.com/aBER0724/crosspoint-reader-cn

# If already cloned without submodules:
git submodule update --init --recursive
```

### Build and Flash

Connect your device and run:

```bash
pio run --target upload
```

### CJK Font Configuration

1. Create a `/fonts/` folder in the SD card root directory
2. Place `.bin` format CJK font files inside
3. Select "External Font" option in Settings

Font filename format: `FontName_size_WxH.bin`
Example: `SourceHanSansCN-Medium_20_20x20.bin`

## 📚 Technical Details

### Data Caching

Book chapters are cached to the SD card on first load. Subsequent loads read from cache. The cache directory is located at `.crosspoint/` on the SD card root:

```
.crosspoint/
├── epub_12471232/       # Each EPUB has a subdirectory (epub_<hash>)
│   ├── progress.bin     # Reading progress
│   ├── cover.bmp        # Book cover
│   ├── book.bin         # Book metadata
│   └── sections/        # Chapter data
│       ├── 0.bin
│       ├── 1.bin
│       └── ...
└── epub_189013891/
```

Delete the `.crosspoint` directory to clear all cache.

For more technical details, see the [File Formats Documentation](./docs/file-formats.md).

## 🤝 Contributing

Issues and Pull Requests are welcome!

### Contribution Process

1. Fork this repository
2. Create a feature branch (`feature/your-feature`)
3. Commit your changes
4. Submit a Pull Request

## 📜 Acknowledgements

- [daveallie/crosspoint-reader](https://github.com/daveallie/crosspoint-reader) - Original CrossPoint Reader

---

**Disclaimer**: This project is not affiliated with Xteink or any manufacturer of the X4 hardware. It is a community project.
