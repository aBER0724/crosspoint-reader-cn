#!/usr/bin/env python3
"""
Convert .bin font file to C header file for embedding in firmware.

Usage:
    python bin_to_header.py SourceHanSansUI_10_14x14.bin -o cjk_ui_font.h

Author: Claude (AI assistant)
"""

import argparse
import os
from pathlib import Path


def bin_to_header(input_file: str, output_file: str, var_name: str = None):
    """Convert binary file to C header with PROGMEM data."""

    # Read binary data
    with open(input_file, 'rb') as f:
        data = f.read()

    # Parse font info from filename (e.g., SourceHanSansUI_10_14x14.bin)
    basename = Path(input_file).stem
    parts = basename.rsplit('_', 2)
    if len(parts) >= 3:
        font_name = parts[0]
        font_size = int(parts[1])
        dims = parts[2].split('x')
        char_width = int(dims[0])
        char_height = int(dims[1])
    else:
        font_name = basename
        font_size = 10
        char_width = 14
        char_height = 14

    if var_name is None:
        var_name = basename.lower().replace('-', '_')

    bytes_per_row = (char_width + 7) // 8
    bytes_per_char = bytes_per_row * char_height

    # Generate header
    header = f'''/**
 * Auto-generated CJK UI font data
 * Source: {os.path.basename(input_file)}
 * Font: {font_name}
 * Size: {font_size}pt
 * Dimensions: {char_width}x{char_height}
 * Bytes per char: {bytes_per_char}
 * Total size: {len(data)} bytes ({len(data) / 1024 / 1024:.2f} MB)
 */
#pragma once

#include <cstdint>
#include <pgmspace.h>

// Font parameters
static constexpr uint8_t CJK_UI_FONT_WIDTH = {char_width};
static constexpr uint8_t CJK_UI_FONT_HEIGHT = {char_height};
static constexpr uint8_t CJK_UI_FONT_BYTES_PER_ROW = {bytes_per_row};
static constexpr uint8_t CJK_UI_FONT_BYTES_PER_CHAR = {bytes_per_char};
static constexpr uint32_t CJK_UI_FONT_MAX_CODEPOINT = {len(data) // bytes_per_char - 1};

// Font bitmap data (stored in Flash/PROGMEM)
static const uint8_t CJK_UI_FONT_DATA[] PROGMEM = {{
'''

    # Write data in hex format
    line_width = 16  # bytes per line
    for i in range(0, len(data), line_width):
        chunk = data[i:i + line_width]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        header += f'    {hex_str},\n'

    header += '''};

/**
 * Get glyph bitmap for a codepoint from built-in CJK UI font.
 * @param cp Unicode codepoint
 * @return Pointer to glyph bitmap in PROGMEM, or nullptr if not available
 */
inline const uint8_t* getCjkUiGlyph(uint32_t cp) {
    if (cp > CJK_UI_FONT_MAX_CODEPOINT) {
        return nullptr;
    }
    uint32_t offset = cp * CJK_UI_FONT_BYTES_PER_CHAR;
    return &CJK_UI_FONT_DATA[offset];
}

/**
 * Check if a codepoint has a glyph in the built-in CJK UI font.
 * Note: This only checks if the codepoint is in range, not if the glyph is non-empty.
 */
inline bool hasCjkUiGlyph(uint32_t cp) {
    return cp <= CJK_UI_FONT_MAX_CODEPOINT;
}
'''

    # Write output
    with open(output_file, 'w') as f:
        f.write(header)

    print(f"Generated: {output_file}")
    print(f"Font: {font_name} {font_size}pt ({char_width}x{char_height})")
    print(f"Data size: {len(data)} bytes ({len(data) / 1024 / 1024:.2f} MB)")


def main():
    parser = argparse.ArgumentParser(description="Convert .bin font to C header")
    parser.add_argument("input", help="Input .bin file")
    parser.add_argument("-o", "--output", help="Output .h file")
    parser.add_argument("-n", "--name", help="Variable name prefix")
    args = parser.parse_args()

    output = args.output or Path(args.input).stem + '.h'
    bin_to_header(args.input, output, args.name)


if __name__ == "__main__":
    main()
