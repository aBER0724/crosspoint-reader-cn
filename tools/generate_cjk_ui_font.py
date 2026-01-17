#!/usr/bin/env python3
"""
Generate optimized CJK UI font header with only required characters.
Uses a lookup table instead of full Unicode range to minimize size.

Usage:
    python generate_cjk_ui_font.py font.otf --size 10 --width 14 --height 14

Author: Claude (AI assistant)
"""

import argparse
import freetype
from pathlib import Path


def get_ui_chars():
    """Get all characters needed for UI (Chinese + Japanese + English + common symbols)."""
    ui_strings = [
        # English
        "CrossPoint", "BOOTING", "SLEEPING", "Browse Files", "File Transfer", "Settings",
        "Calibre Library", "Continue Reading", "No open book", "Start reading below",
        "Books", "No books found", "Select Chapter", "No chapters", "End of book",
        "Empty chapter", "Indexing...", "Memory error", "Page load error", "Empty file",
        "Out of bounds", "WiFi Networks", "No networks found", "Scanning...", "Connecting...",
        "Connected!", "Connection Failed", "Forget Network?", "Save password for next time?",
        "Remove saved password?", "Press OK to scan again", "Press any button to continue",
        "LEFT/RIGHT: Select | OK: Confirm", "How would you like to connect?",
        "Join a Network", "Create Hotspot", "Connect to an existing WiFi network",
        "Create a WiFi network others can join", "Starting Hotspot...", "Hotspot Mode",
        "Connect your device to this WiFi network", "Open this URL in your browser",
        "Calibre Wireless", "Calibre Web URL", "Connect as Wireless Device",
        "* = Encrypted | + = Saved", "MAC address:",
        "Sleep Screen", "Sleep Screen Cover Mode", "Status Bar", "Hide Battery %",
        "Extra Paragraph Spacing", "Text Anti-Aliasing", "Short Power Button Click",
        "Reading Orientation", "Front Button Layout", "Side Button Layout (reader)",
        "Long-press Chapter Skip", "Reader Font Family", "Reader Font", "Reader Font Size",
        "Reader Line Spacing", "Reader Screen Margin", "Reader Paragraph Alignment",
        "Time to Sleep", "Refresh Frequency", "Calibre Settings", "Check for updates",
        "Language", "Dark", "Light", "Custom", "Cover", "None", "Fit", "Crop",
        "No Progress", "Full", "Never", "In Reader", "Always", "Ignore", "Sleep",
        "Page Turn", "Portrait", "Landscape CW", "Inverted", "Landscape CCW",
        "Bck, Cnfrm, Lft, Rght", "Lft, Rght, Bck, Cnfrm", "Lft, Bck, Cnfrm, Rght",
        "Prev, Next", "Next, Prev", "Small", "Medium", "Large", "X Large",
        "Tight", "Normal", "Wide", "Justify", "Left", "Center", "Right",
        "1 min", "5 min", "10 min", "15 min", "30 min",
        "1 page", "5 pages", "10 pages", "15 pages", "30 pages",
        "Update", "Checking for updates...", "New update available!",
        "Current version", "New version", "Updating...", "Up to date",
        "Update failed", "Update complete", "Long-press power button to turn on",
        "External Font", "Built-in (Disabled)", "No entries found", "Downloading...",
        "Error", "Back", "Exit", "Home", "Save", "Select", "Toggle", "OK", "Cancel",
        "Connect", "Open", "Retry", "Yes", "No", "ON", "OFF", "English", "Chinese",
        "Japanese", "QR", "URL", "WiFi", "0123456789", "!@#$%^&*()_+-=[]{}|;:',.<>?/~`",
        # Chinese
        "启动中", "休眠中", "浏览文件", "文件传输", "设置", "书库", "继续阅读",
        "无打开的书籍", "从下方开始阅读", "书籍", "未找到书籍", "选择章节",
        "无章节", "已到书末", "空章节", "索引中", "内存错误", "页面加载错误",
        "空文件", "超出范围", "网络", "未找到网络", "扫描中", "连接中",
        "已连接", "连接失败", "忘记网络", "保存密码", "删除已保存密码",
        "按确定重新扫描", "按任意键继续", "左右选择确定确认", "选择连接方式",
        "加入网络", "创建热点", "连接到现有网络", "创建网络供他人连接",
        "启动热点中", "热点模式", "将设备连接到此", "在浏览器中打开此",
        "或用手机扫描二维码", "无线连接", "地址", "作为无线设备连接",
        "加密", "已保存", "休眠屏幕", "封面显示模式", "状态栏", "隐藏电量百分比",
        "段落额外间距", "文字抗锯齿", "电源键短按", "阅读方向", "前置按钮布局",
        "侧边按钮布局", "长按跳转章节", "阅读字体", "外置中文字体", "字体大小",
        "行间距", "页面边距", "段落对齐", "休眠时间", "刷新频率", "检查更新",
        "语言", "深色", "浅色", "自定义", "封面", "无", "适应", "裁剪",
        "无进度", "完整", "从不", "阅读时", "始终", "忽略", "休眠", "翻页",
        "竖屏", "横屏顺时针", "倒置", "横屏逆时针", "上一页下一页", "小", "中",
        "大", "特大", "紧凑", "正常", "宽松", "两端对齐", "左对齐", "居中",
        "右对齐", "分钟", "页", "更新", "检查更新中", "有新版本可用",
        "当前版本", "新版本", "更新中", "已是最新版本", "更新失败", "更新完成",
        "长按电源键开机", "外置字体", "内置已禁用", "无条目", "下载中", "错误",
        "返回", "退出", "主页", "保存", "选择", "切换", "确定", "取消",
        "连接", "打开", "重试", "是", "否", "开", "关", "中文", "地址",
        # Japanese
        "起動中", "スリープ中", "ファイル一覧", "ファイル転送", "設定",
        "ライブラリ", "続きを読む", "開いている本はありません", "下から読書を始める",
        "本", "本が見つかりません", "章を選択", "章なし", "本の終わり", "空の章",
        "インデックス中", "メモリエラー", "ページ読み込みエラー", "空のファイル",
        "範囲外", "ネットワーク", "ネットワークが見つかりません", "スキャン中",
        "接続中", "接続完了", "接続失敗", "ネットワークを忘れる", "パスワードを保存",
        "保存したパスワードを削除", "を押して再スキャン", "任意のボタンを押して続行",
        "選択確認", "接続方法を選択", "ネットワークに参加", "ホットスポットを作成",
        "既存のに接続", "を作成して他の人が接続", "ホットスポットを起動中",
        "ホットスポットモード", "このにデバイスを接続", "ブラウザでこのを開く",
        "スマホでコードをスキャン", "ワイヤレス", "アドレス", "ワイヤレスデバイスとして接続",
        "暗号化", "保存済み", "スリープ画面", "スリープ画面カバーモード",
        "ステータスバー", "バッテリーを隠す", "段落追加間隔", "テキストアンチエイリアス",
        "電源ボタン短押し", "読書方向", "前面ボタン配置", "側面ボタン配置",
        "長押し章スキップ", "フォントファミリー", "外部中国語フォント", "フォントサイズ",
        "行間隔", "画面余白", "段落配置", "スリープまでの時間", "リフレッシュ頻度",
        "アップデートを確認", "言語", "ダーク", "ライト", "カスタム", "カバー",
        "なし", "フィット", "クロップ", "進捗なし", "フル", "しない", "読書中",
        "常に", "無視", "スリープ", "ページめくり", "縦向き", "横向き時計回り",
        "反転", "横向き反時計回り", "前次", "次前", "小", "中", "大", "特大",
        "狭い", "普通", "広い", "両端揃え", "左揃え", "中央揃え", "右揃え",
        "分", "ページ", "アップデート", "アップデートを確認中", "新しいアップデートがあります",
        "現在のバージョン", "新バージョン", "アップデート中", "最新です",
        "アップデート失敗", "アップデート完了", "電源ボタンを長押ししてオン",
        "外部フォント", "内蔵無効", "エントリがありません", "ダウンロード中", "エラー",
        "戻る", "終了", "ホーム", "保存", "選択", "切替", "確認", "キャンセル",
        "接続", "開く", "再試行", "はい", "いいえ", "日本語",
    ]

    chars = set()
    for s in ui_strings:
        for c in s:
            cp = ord(c)
            # Include both CJK characters and ASCII/Latin characters
            if cp >= 0x20:  # Include printable ASCII and above
                chars.add(cp)
    return sorted(chars)


def render_glyph(face, codepoint, char_width, char_height):
    """Render a single glyph to a 1-bit bitmap.

    Returns:
        tuple: (bitmap_data, actual_width) or None if glyph not found
        - bitmap_data: bytes of the rendered glyph
        - actual_width: actual advance width in pixels (for proportional spacing)
    """
    glyph_index = face.get_char_index(codepoint)
    if glyph_index == 0:
        return None

    try:
        face.load_glyph(glyph_index, freetype.FT_LOAD_RENDER)
    except Exception:
        return None

    bitmap = face.glyph.bitmap
    if bitmap.width == 0 or bitmap.rows == 0:
        return None

    # Get actual advance width (in 1/64 pixels, convert to pixels)
    actual_width = int(face.glyph.advance.x / 64)

    # For CJK characters (full-width), use fixed width
    # For ASCII/Latin characters, use actual width for proportional spacing
    if codepoint >= 0x3000:  # CJK range
        actual_width = char_width

    left = face.glyph.bitmap_left
    top = face.glyph.bitmap_top
    baseline_y = int(face.size.ascender / 64)
    glyph_x = max(0, left)
    glyph_y = max(0, baseline_y - top)

    bytes_per_row = (char_width + 7) // 8
    buffer = bytearray(bytes_per_row * char_height)

    for gy in range(bitmap.rows):
        dst_y = glyph_y + gy
        if dst_y >= char_height:
            break
        for gx in range(bitmap.width):
            dst_x = glyph_x + gx
            if dst_x >= char_width:
                break
            src_byte = bitmap.buffer[gy * bitmap.pitch + gx]
            if src_byte >= 64:
                byte_idx = dst_y * bytes_per_row + (dst_x // 8)
                bit_idx = 7 - (dst_x % 8)
                buffer[byte_idx] |= (1 << bit_idx)

    return (bytes(buffer), actual_width)


def main():
    parser = argparse.ArgumentParser(description="Generate optimized CJK UI font header")
    parser.add_argument("font", help="Path to TTF/OTF font file")
    parser.add_argument("--size", type=int, default=10, help="Font size in points")
    parser.add_argument("--width", type=int, default=14, help="Character width")
    parser.add_argument("--height", type=int, default=14, help="Character height")
    parser.add_argument("--dpi", type=int, default=96, help="DPI for rendering")
    parser.add_argument("-o", "--output", default="lib/GfxRenderer/cjk_ui_font.h", help="Output file")
    args = parser.parse_args()

    print(f"Loading font: {args.font}")
    face = freetype.Face(args.font)
    face.set_char_size(args.size << 6, args.size << 6, args.dpi, args.dpi)

    char_width = args.width
    char_height = args.height
    bytes_per_row = (char_width + 7) // 8
    bytes_per_char = bytes_per_row * char_height

    print(f"Character size: {char_width}x{char_height}")
    print(f"Bytes per character: {bytes_per_char}")

    # Get required characters
    ui_chars = get_ui_chars()
    print(f"Required CJK characters: {len(ui_chars)}")

    # Render glyphs
    glyphs = {}
    glyph_widths = {}
    for cp in ui_chars:
        result = render_glyph(face, cp, char_width, char_height)
        if result:
            glyph_data, actual_width = result
            glyphs[cp] = glyph_data
            glyph_widths[cp] = actual_width

    print(f"Rendered glyphs: {len(glyphs)}")

    # Calculate total data size
    total_data_size = len(glyphs) * bytes_per_char
    print(f"Total data size: {total_data_size} bytes ({total_data_size / 1024:.1f} KB)")

    # Generate header file
    # Determine namespace based on font size
    namespace_name = f"CjkUiFont{char_width}"

    header = f'''/**
 * Auto-generated CJK UI font data (optimized - UI characters only)
 * Font: {Path(args.font).stem}
 * Size: {args.size}pt
 * Dimensions: {char_width}x{char_height}
 * Characters: {len(glyphs)}
 * Total size: {total_data_size} bytes ({total_data_size / 1024:.1f} KB)
 *
 * This is a sparse font containing only UI-required CJK characters.
 * Uses a lookup table for codepoint -> glyph index mapping.
 * Supports proportional spacing for English characters.
 */
#pragma once
namespace {namespace_name} {{

#include <cstdint>
#include <pgmspace.h>

// Font parameters
static constexpr uint8_t CJK_UI_FONT_WIDTH = {char_width};
static constexpr uint8_t CJK_UI_FONT_HEIGHT = {char_height};
static constexpr uint8_t CJK_UI_FONT_BYTES_PER_ROW = {bytes_per_row};
static constexpr uint8_t CJK_UI_FONT_BYTES_PER_CHAR = {bytes_per_char};
static constexpr uint16_t CJK_UI_FONT_GLYPH_COUNT = {len(glyphs)};

// Codepoint lookup table (sorted for binary search)
static const uint16_t CJK_UI_CODEPOINTS[] PROGMEM = {{
'''

    # Write codepoint lookup table
    codepoints = sorted(glyphs.keys())
    for i in range(0, len(codepoints), 16):
        chunk = codepoints[i:i+16]
        hex_str = ', '.join(f'0x{cp:04X}' for cp in chunk)
        header += f'    {hex_str},\n'

    header += '''};

// Glyph width table (actual advance width for proportional spacing)
static const uint8_t CJK_UI_GLYPH_WIDTHS[] PROGMEM = {
'''

    # Write glyph widths in order
    for i in range(0, len(codepoints), 16):
        chunk = codepoints[i:i+16]
        width_str = ', '.join(f'{glyph_widths[cp]:2d}' for cp in chunk)
        header += f'    {width_str},\n'

    header += '''};

// Glyph bitmap data (stored in Flash/PROGMEM)
static const uint8_t CJK_UI_FONT_DATA[] PROGMEM = {
'''

    # Write glyph data in order
    for cp in codepoints:
        glyph_data = glyphs[cp]
        hex_str = ', '.join(f'0x{b:02X}' for b in glyph_data)
        header += f'    // U+{cp:04X} ({chr(cp)})\n'
        header += f'    {hex_str},\n'

    header += '''};

/**
 * Binary search for codepoint in lookup table.
 * @return Index of glyph, or -1 if not found
 */
inline int16_t findCjkUiGlyphIndex(uint16_t cp) {
    int16_t lo = 0;
    int16_t hi = CJK_UI_FONT_GLYPH_COUNT - 1;

    while (lo <= hi) {
        int16_t mid = (lo + hi) / 2;
        uint16_t midCp = pgm_read_word(&CJK_UI_CODEPOINTS[mid]);

        if (midCp == cp) {
            return mid;
        } else if (midCp < cp) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/**
 * Get glyph bitmap for a codepoint from built-in CJK UI font.
 * @param cp Unicode codepoint
 * @return Pointer to glyph bitmap in PROGMEM, or nullptr if not available
 */
inline const uint8_t* getCjkUiGlyph(uint32_t cp) {
    if (cp > 0xFFFF) return nullptr;  // Only BMP supported

    int16_t idx = findCjkUiGlyphIndex(static_cast<uint16_t>(cp));
    if (idx < 0) return nullptr;

    return &CJK_UI_FONT_DATA[idx * CJK_UI_FONT_BYTES_PER_CHAR];
}

/**
 * Check if a codepoint has a glyph in the built-in CJK UI font.
 */
inline bool hasCjkUiGlyph(uint32_t cp) {
    if (cp > 0xFFFF) return false;
    return findCjkUiGlyphIndex(static_cast<uint16_t>(cp)) >= 0;
}

/**
 * Get actual advance width for a codepoint (for proportional spacing).
 * @param cp Unicode codepoint
 * @return Advance width in pixels, or 0 if not available
 */
inline uint8_t getCjkUiGlyphWidth(uint32_t cp) {
    if (cp > 0xFFFF) return 0;

    int16_t idx = findCjkUiGlyphIndex(static_cast<uint16_t>(cp));
    if (idx < 0) return 0;

    return pgm_read_byte(&CJK_UI_GLYPH_WIDTHS[idx]);
}

}  // namespace ''' + namespace_name + '''
'''

    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(header)

    print(f"\nGenerated: {args.output}")
    print(f"Header file size: {len(header) / 1024:.1f} KB")


if __name__ == "__main__":
    main()
