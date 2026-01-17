#!/usr/bin/env python3
"""
Generate external font .bin file for CrossPoint Reader UI.

The .bin format is a simple bitmap font where:
- Each character is stored at offset = codepoint * bytes_per_char
- Each character is a 1-bit bitmap with rows packed into bytes
- File must be large enough to contain the highest codepoint needed

Usage:
    python generate_ui_font.py font.otf --size 12 --output SourceHanSans_12_16x18.bin

Author: Claude (AI assistant)
"""

import argparse
import freetype
import sys
from pathlib import Path


def render_glyph(face, codepoint, char_width, char_height):
    """
    Render a single glyph to a 1-bit bitmap.
    Returns bytes for the character, or None if glyph doesn't exist.
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
        # Space or empty glyph
        return None

    # Calculate positioning to center the glyph
    left = face.glyph.bitmap_left
    top = face.glyph.bitmap_top

    # Ascender baseline position (from top of cell)
    baseline_y = int(face.size.ascender / 64)

    # Glyph start position in the cell
    glyph_x = max(0, left)
    glyph_y = max(0, baseline_y - top)

    # Create bitmap buffer
    bytes_per_row = (char_width + 7) // 8
    buffer = bytearray(bytes_per_row * char_height)

    # Copy glyph bitmap to buffer
    for gy in range(bitmap.rows):
        dst_y = glyph_y + gy
        if dst_y >= char_height:
            break

        for gx in range(bitmap.width):
            dst_x = glyph_x + gx
            if dst_x >= char_width:
                break

            # Get source pixel (8-bit grayscale)
            src_byte = bitmap.buffer[gy * bitmap.pitch + gx]

            # Threshold to 1-bit (128 = 50%)
            if src_byte >= 64:  # Lower threshold for better visibility
                byte_idx = dst_y * bytes_per_row + (dst_x // 8)
                bit_idx = 7 - (dst_x % 8)
                buffer[byte_idx] |= (1 << bit_idx)

    return bytes(buffer)


def get_cjk_ranges():
    """Get Unicode ranges for CJK characters."""
    return [
        (0x4E00, 0x9FFF),   # CJK Unified Ideographs
        (0x3040, 0x309F),   # Hiragana
        (0x30A0, 0x30FF),   # Katakana
        (0x3000, 0x303F),   # CJK Symbols and Punctuation
        (0xFF00, 0xFFEF),   # Halfwidth and Fullwidth Forms
    ]


def extract_chars_from_strings(strings):
    """Extract unique characters from a list of strings."""
    chars = set()
    for s in strings:
        for c in s:
            chars.add(ord(c))
    return sorted(chars)


def get_ui_chars():
    """Get all characters needed for UI (Chinese + Japanese + common symbols)."""
    # UI strings from I18n.cpp (manually extracted)
    ui_strings = [
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
        "连接", "打开", "重试", "是", "否", "开", "关", "中文",
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
        # Common symbols and punctuation
        "0123456789", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
        "!@#$%^&*()_+-=[]{}|;':\",./<>?`~", "« »", "...", "×÷±",
    ]

    return extract_chars_from_strings(ui_strings)


def main():
    parser = argparse.ArgumentParser(description="Generate external font .bin file")
    parser.add_argument("font", help="Path to TTF/OTF font file")
    parser.add_argument("--size", type=int, default=12, help="Font size in points")
    parser.add_argument("--width", type=int, help="Character width (auto if not specified)")
    parser.add_argument("--height", type=int, help="Character height (auto if not specified)")
    parser.add_argument("--output", "-o", help="Output filename (auto-generated if not specified)")
    parser.add_argument("--max-codepoint", type=int, default=0x9FFF,
                        help="Maximum codepoint to support (default: 0x9FFF for CJK)")
    parser.add_argument("--dpi", type=int, default=150, help="DPI for rendering (default: 150)")
    args = parser.parse_args()

    # Load font
    print(f"Loading font: {args.font}")
    face = freetype.Face(args.font)
    face.set_char_size(args.size << 6, args.size << 6, args.dpi, args.dpi)

    # Calculate character dimensions
    if args.width and args.height:
        char_width = args.width
        char_height = args.height
    else:
        # Auto-calculate from font metrics
        char_height = int((face.size.ascender - face.size.descender) / 64) + 2
        # For CJK fonts, width is usually same as height or slightly less
        char_width = char_height

        # Try rendering some CJK characters to verify
        test_chars = "中文日本語"
        max_width = 0
        for c in test_chars:
            glyph_index = face.get_char_index(ord(c))
            if glyph_index:
                face.load_glyph(glyph_index, freetype.FT_LOAD_RENDER)
                max_width = max(max_width, face.glyph.bitmap.width)

        if max_width > 0:
            char_width = max_width + 2

    print(f"Character size: {char_width}x{char_height}")

    # Get font name from filename
    font_name = Path(args.font).stem.replace(" ", "").replace("-", "")

    # Generate output filename
    if args.output:
        output_path = args.output
    else:
        output_path = f"{font_name}_{args.size}_{char_width}x{char_height}.bin"

    print(f"Output: {output_path}")

    # Get characters to render
    ui_chars = get_ui_chars()
    print(f"Total UI characters: {len(ui_chars)}")

    # Calculate file size
    bytes_per_row = (char_width + 7) // 8
    bytes_per_char = bytes_per_row * char_height
    max_codepoint = args.max_codepoint
    file_size = (max_codepoint + 1) * bytes_per_char

    print(f"Bytes per character: {bytes_per_char}")
    print(f"Max codepoint: 0x{max_codepoint:X}")
    print(f"File size: {file_size / 1024 / 1024:.2f} MB")

    # Create output buffer
    output = bytearray(file_size)

    # Render each character
    rendered = 0
    skipped = 0

    for codepoint in ui_chars:
        if codepoint > max_codepoint:
            skipped += 1
            continue

        glyph_data = render_glyph(face, codepoint, char_width, char_height)
        if glyph_data:
            offset = codepoint * bytes_per_char
            output[offset:offset + bytes_per_char] = glyph_data
            rendered += 1
        else:
            skipped += 1

    print(f"Rendered: {rendered}, Skipped: {skipped}")

    # Write output file
    with open(output_path, "wb") as f:
        f.write(output)

    print(f"Done! Output: {output_path}")
    print(f"\nTo use this font:")
    print(f"1. Copy {output_path} to SD card /.fonts/ directory")
    print(f"2. Select it in Settings > External Chinese Font")


if __name__ == "__main__":
    main()
