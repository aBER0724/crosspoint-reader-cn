#pragma once

#include <cstdint>

/**
 * Internationalization (i18n) system for CrossPoint Reader
 * Supports English and Chinese UI languages
 */

// String IDs - organized by category
enum class StrId : uint16_t {
  // === Boot/Sleep ===
  CROSSPOINT, // "CrossPoint"
  BOOTING,    // "BOOTING" / "启动中"
  SLEEPING,   // "SLEEPING" / "休眠中"

  // === Home Menu ===
  BROWSE_FILES,     // "Browse Files" / "浏览文件"
  FILE_TRANSFER,    // "File Transfer" / "文件传输"
  SETTINGS_TITLE,   // "Settings" / "设置"
  CALIBRE_LIBRARY,  // "Calibre Library" / "Calibre书库"
  CONTINUE_READING, // "Continue Reading" / "继续阅读"
  NO_OPEN_BOOK,     // "No open book" / "无打开的书籍"
  START_READING,    // "Start reading below" / "从下方开始阅读"

  // === File Browser ===
  BOOKS,          // "Books" / "书籍"
  NO_BOOKS_FOUND, // "No books found" / "未找到书籍"

  // === Reader ===
  SELECT_CHAPTER,  // "Select Chapter" / "选择章节"
  NO_CHAPTERS,     // "No chapters" / "无章节"
  END_OF_BOOK,     // "End of book" / "已到书末"
  EMPTY_CHAPTER,   // "Empty chapter" / "空章节"
  INDEXING,        // "Indexing..." / "索引中..."
  MEMORY_ERROR,    // "Memory error" / "内存错误"
  PAGE_LOAD_ERROR, // "Page load error" / "页面加载错误"
  EMPTY_FILE,      // "Empty file" / "空文件"
  OUT_OF_BOUNDS,   // "Out of bounds" / "超出范围"

  // === Network ===
  WIFI_NETWORKS,      // "WiFi Networks" / "WiFi网络"
  NO_NETWORKS,        // "No networks found" / "未找到网络"
  SCANNING,           // "Scanning..." / "扫描中..."
  CONNECTING,         // "Connecting..." / "连接中..."
  CONNECTED,          // "Connected!" / "已连接!"
  CONNECTION_FAILED,  // "Connection Failed" / "连接失败"
  FORGET_NETWORK,     // "Forget Network?" / "忘记网络?"
  SAVE_PASSWORD,      // "Save password for next time?" / "保存密码?"
  REMOVE_PASSWORD,    // "Remove saved password?" / "删除已保存密码?"
  PRESS_OK_SCAN,      // "Press OK to scan again" / "按确定重新扫描"
  PRESS_ANY_CONTINUE, // "Press any button to continue" / "按任意键继续"
  SELECT_HINT,  // "LEFT/RIGHT: Select | OK: Confirm" / "左/右:选择 | 确定:确认"
  HOW_CONNECT,  // "How would you like to connect?" / "选择连接方式"
  JOIN_NETWORK, // "Join a Network" / "加入网络"
  CREATE_HOTSPOT, // "Create Hotspot" / "创建热点"
  JOIN_DESC,    // "Connect to an existing WiFi network" / "连接到现有WiFi网络"
  HOTSPOT_DESC, // "Create a WiFi network others can join" /
                // "创建WiFi网络供他人连接"
  STARTING_HOTSPOT,  // "Starting Hotspot..." / "启动热点中..."
  HOTSPOT_MODE,      // "Hotspot Mode" / "热点模式"
  CONNECT_WIFI_HINT, // "Connect your device to this WiFi network" /
                     // "将设备连接到此WiFi"
  OPEN_URL_HINT,     // "Open this URL in your browser" / "在浏览器中打开此URL"
  SCAN_QR_HINT, // "or scan QR code with your phone:" / "或用手机扫描二维码:"
  CALIBRE_WIRELESS, // "Calibre Wireless" / "Calibre无线连接"
  CALIBRE_WEB_URL,  // "Calibre Web URL" / "Calibre Web地址"
  CONNECT_WIRELESS, // "Connect as Wireless Device" / "作为无线设备连接"
  NETWORK_LEGEND,   // "* = Encrypted | + = Saved" / "* = 加密 | + = 已保存"
  MAC_ADDRESS,      // "MAC address:" / "MAC地址:"

  // === Settings ===
  SLEEP_SCREEN,     // "Sleep Screen" / "休眠屏幕"
  SLEEP_COVER_MODE, // "Sleep Screen Cover Mode" / "封面显示模式"
  STATUS_BAR,       // "Status Bar" / "状态栏"
  HIDE_BATTERY,     // "Hide Battery %" / "隐藏电量百分比"
  EXTRA_SPACING,    // "Extra Paragraph Spacing" / "段落额外间距"
  TEXT_AA,          // "Text Anti-Aliasing" / "文字抗锯齿"
  SHORT_PWR_BTN,    // "Short Power Button Click" / "电源键短按"
  ORIENTATION,      // "Reading Orientation" / "阅读方向"
  FRONT_BTN_LAYOUT, // "Front Button Layout" / "前置按钮布局"
  SIDE_BTN_LAYOUT,  // "Side Button Layout (reader)" / "侧边按钮布局"
  LONG_PRESS_SKIP,  // "Long-press Chapter Skip" / "长按跳转章节"
  FONT_FAMILY,      // "Reader Font Family" / "阅读字体"
  EXT_CHINESE_FONT, // "External Chinese Font" / "阅读器字体"
  EXT_UI_FONT,      // "External UI Font" / "UI字体"
  FONT_SIZE,        // "Reader Font Size" / "字体大小"
  LINE_SPACING,     // "Reader Line Spacing" / "行间距"
  SCREEN_MARGIN,    // "Reader Screen Margin" / "页面边距"
  PARA_ALIGNMENT,   // "Reader Paragraph Alignment" / "段落对齐"
  TIME_TO_SLEEP,    // "Time to Sleep" / "休眠时间"
  REFRESH_FREQ,     // "Refresh Frequency" / "刷新频率"
  CALIBRE_SETTINGS, // "Calibre Settings" / "Calibre设置"
  CHECK_UPDATES,    // "Check for updates" / "检查更新"
  LANGUAGE,         // "Language" / "语言"
  SELECT_WALLPAPER, // "Select Wallpaper" / "选择壁纸"

  // Setting Values
  DARK,          // "Dark" / "深色"
  LIGHT,         // "Light" / "浅色"
  CUSTOM,        // "Custom" / "自定义"
  COVER,         // "Cover" / "封面"
  NONE,          // "None" / "无"
  FIT,           // "Fit" / "适应"
  CROP,          // "Crop" / "裁剪"
  NO_PROGRESS,   // "No Progress" / "无进度"
  FULL,          // "Full" / "完整"
  NEVER,         // "Never" / "从不"
  IN_READER,     // "In Reader" / "阅读时"
  ALWAYS,        // "Always" / "始终"
  IGNORE,        // "Ignore" / "忽略"
  SLEEP,         // "Sleep" / "休眠"
  PAGE_TURN,     // "Page Turn" / "翻页"
  PORTRAIT,      // "Portrait" / "竖屏"
  LANDSCAPE_CW,  // "Landscape CW" / "横屏顺时针"
  INVERTED,      // "Inverted" / "倒置"
  LANDSCAPE_CCW, // "Landscape CCW" / "横屏逆时针"
  PREV_NEXT,     // "Prev/Next" / "上一页/下一页"
  NEXT_PREV,     // "Next/Prev" / "下一页/上一页"
  BOOKERLY,      // "Bookerly"
  NOTO_SANS,     // "Noto Sans"
  OPEN_DYSLEXIC, // "Open Dyslexic"
  SMALL,         // "Small" / "小"
  MEDIUM,        // "Medium" / "中"
  LARGE,         // "Large" / "大"
  X_LARGE,       // "X Large" / "特大"
  TIGHT,         // "Tight" / "紧凑"
  NORMAL,        // "Normal" / "正常"
  WIDE,          // "Wide" / "宽松"
  JUSTIFY,       // "Justify" / "两端对齐"
  LEFT,          // "Left" / "左对齐"
  CENTER,        // "Center" / "居中"
  RIGHT,         // "Right" / "右对齐"
  MIN_1,         // "1 min" / "1分钟"
  MIN_5,         // "5 min" / "5分钟"
  MIN_10,        // "10 min" / "10分钟"
  MIN_15,        // "15 min" / "15分钟"
  MIN_30,        // "30 min" / "30分钟"
  PAGES_1,       // "1 page" / "1页"
  PAGES_5,       // "5 pages" / "5页"
  PAGES_10,      // "10 pages" / "10页"
  PAGES_15,      // "15 pages" / "15页"
  PAGES_30,      // "30 pages" / "30页"

  // === OTA Update ===
  UPDATE,          // "Update" / "更新"
  CHECKING_UPDATE, // "Checking for update..." / "检查更新中..."
  NEW_UPDATE,      // "New update available!" / "有新版本可用!"
  CURRENT_VERSION, // "Current Version: " / "当前版本: "
  NEW_VERSION,     // "New Version: " / "新版本: "
  UPDATING,        // "Updating..." / "更新中..."
  NO_UPDATE,       // "No update available" / "已是最新版本"
  UPDATE_FAILED,   // "Update failed" / "更新失败"
  UPDATE_COMPLETE, // "Update complete" / "更新完成"
  POWER_ON_HINT,   // "Press and hold power button to turn back on" /
                   // "长按电源键开机"

  // === Font Selection ===
  EXTERNAL_FONT,    // "External Font" / "外置字体"
  BUILTIN_DISABLED, // "Built-in (Disabled)" / "内置(已禁用)"

  // === OPDS Browser ===
  NO_ENTRIES,        // "No entries found" / "无条目"
  DOWNLOADING,       // "Downloading..." / "下载中..."
  ERROR,             // "Error:" / "错误:"
  UNNAMED,           // "Unnamed" / "未命名"
  NETWORK_PREFIX,    // "Network: " / "网络: "
  IP_ADDRESS_PREFIX, // "IP Address: " / "IP地址: "
  SCAN_QR_WIFI_HINT, // "or scan QR code with your phone to connect to Wifi." /
                     // "或用手机扫描二维码连接WiFi"

  // === Buttons ===
  BACK,    // "« Back" / "« 返回"
  EXIT,    // "« Exit" / "« 退出"
  HOME,    // "« Home" / "« 主页"
  SAVE,    // "« Save" / "« 保存"
  SELECT,  // "Select" / "选择"
  TOGGLE,  // "Toggle" / "切换"
  CONFIRM, // "Confirm" / "确定"
  CANCEL,  // "Cancel" / "取消"
  CONNECT, // "Connect" / "连接"
  OPEN,    // "Open" / "打开"
  RETRY,   // "Retry" / "重试"
  YES,     // "Yes" / "是"
  NO,      // "No" / "否"
  ON,      // "ON" / "开"
  OFF,     // "OFF" / "关"

  // === Languages ===
  ENGLISH,  // "English"
  CHINESE,  // "中文"
  JAPANESE, // "日本語"

  // Sentinel - must be last
  _COUNT
};

// Language enum
enum class Language : uint8_t {
  ENGLISH = 0,
  CHINESE = 1,
  JAPANESE = 2,
  _COUNT
};

class I18n {
public:
  static I18n &getInstance();

  // Disable copy
  I18n(const I18n &) = delete;
  I18n &operator=(const I18n &) = delete;

  /**
   * Get localized string by ID
   */
  const char *get(StrId id) const;

  /**
   * Shorthand operator for get()
   */
  const char *operator[](StrId id) const { return get(id); }

  /**
   * Get/Set current language
   */
  Language getLanguage() const { return _language; }
  void setLanguage(Language lang);

  /**
   * Save/Load language setting
   */
  void saveSettings();
  void loadSettings();

  /**
   * Get all unique characters used in a specific language
   * Returns a sorted string of unique characters
   */
  static const char *getCharacterSet(Language lang);

  // String arrays (public for static_assert access)
  static const char *const STRINGS_EN[];
  static const char *const STRINGS_ZH[];
  static const char *const STRINGS_JA[];

private:
  I18n() : _language(Language::ENGLISH) {}

  Language _language;
};

// Convenience macros
#define TR(id) I18n::getInstance().get(StrId::id)
#define I18N I18n::getInstance()
