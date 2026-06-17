// ============================================================
// Panel.h - GDI+ 右侧交互面板
// 封装按钮管理、绘制、点击检测
// 从 main.cpp 提取
// ============================================================
#ifndef Panel_H
#define Panel_H

#include <windows.h>
#include <gdiplus.h>

// ============================================================
// 颜色调色板
// ============================================================
#define CLR_BG_MAIN          RGB(30, 30, 30)
#define CLR_BG_PANEL         RGB(37, 37, 38)
#define CLR_BG_HEADER        RGB(45, 45, 48)
#define CLR_BG_BUTTON        RGB(60, 60, 65)
#define CLR_BG_BUTTON_HOVER  RGB(80, 80, 88)
#define CLR_BG_BUTTON_ACTIVE RGB(0, 122, 204)
#define CLR_BG_KEY           RGB(50, 50, 55)
#define CLR_TEXT              RGB(220, 220, 220)
#define CLR_TEXT_DIM          RGB(160, 160, 165)
#define CLR_TEXT_HEADER       RGB(86, 156, 214)
#define CLR_TEXT_ACCENT       RGB(78, 201, 176)
#define CLR_BORDER            RGB(70, 70, 75)
#define CLR_SEPARATOR         RGB(55, 55, 60)
#define CLR_TOGGLE_ON         RGB(0, 180, 120)
#define CLR_TOGGLE_OFF        RGB(180, 60, 60)

#define PANEL_WIDTH           340
#define PANEL_PADDING         12
#define BTN_HEIGHT            32
#define BTN_SMALL_HEIGHT      28
#define SECTION_GAP           6
#define PANEL_LEFT_PAD        10

// ============================================================
// 按钮类型
// ============================================================
enum BtnType {
    BTN_TOGGLE,      // 切换按钮（线框、投影）
    BTN_DIRECTION,   // 方向按钮对（左/右，上/下）
    BTN_ACTION,      // 单次操作按钮
    BTN_HEADER,      // 分组标题（不可点击）
    BTN_INFO         // 仅信息显示
};

// ============================================================
// 操作 ID
// ============================================================
enum ActionId {
    AID_TOGGLE_WIREFRAME = 1,
    AID_TOGGLE_PROJECTION,
    AID_MODEL_TRANS_X_NEG,  AID_MODEL_TRANS_X_POS,
    AID_MODEL_TRANS_Y_NEG,  AID_MODEL_TRANS_Y_POS,
    AID_MODEL_TRANS_Z_NEG,  AID_MODEL_TRANS_Z_POS,
    AID_MODEL_ROT_X_NEG,    AID_MODEL_ROT_X_POS,
    AID_MODEL_ROT_Y_NEG,    AID_MODEL_ROT_Y_POS,
    AID_MODEL_ROT_Z_NEG,    AID_MODEL_ROT_Z_POS,
    AID_CAMERA_TRANS_X_NEG, AID_CAMERA_TRANS_X_POS,
    AID_CAMERA_TRANS_Y_NEG, AID_CAMERA_TRANS_Y_POS,
    AID_CAMERA_TRANS_Z_NEG, AID_CAMERA_TRANS_Z_POS,
    AID_CAMERA_ROT_X_NEG,   AID_CAMERA_ROT_X_POS,
    AID_CAMERA_ROT_Y_NEG,   AID_CAMERA_ROT_Y_POS,
    AID_CAMERA_ROT_Z_NEG,   AID_CAMERA_ROT_Z_POS,
    AID_SAVE_MODEL,
    AID_RESET_ALL,
    AID_QUIT,
    AID_OPEN_MODEL,

    // ---- 细分相关（新增）----
    AID_SUBDIV_LEVEL_0,
    AID_SUBDIV_LEVEL_1,
    AID_SUBDIV_LEVEL_2,
    AID_SUBDIV_LEVEL_3,
    AID_SUBDIV_LEVEL_4,
    AID_SUBDIV_ALGO_LOOP,      // 选择 Loop 算法
    AID_SUBDIV_ALGO_CATMULL,   // 选择 Catmull-Clark 算法
    AID_SUBDIV_ALGO_BUTTERFLY, // 选择 Butterfly 算法 (高级)
    AID_SUBDIV_ALGORITHM,      // 切换细分算法 (键盘 M 键)
    AID_SUBDIV_DECR_LEVEL,     // 降低细分级别
    AID_SUBDIV_INCR_LEVEL      // 提高细分级别
};

// ============================================================
// GButton - 按钮数据
// ============================================================
struct GButton {
    RECT rect;
    BtnType type;
    const wchar_t* label;
    const wchar_t* keyText;
    const wchar_t* keyText2;    // 方向按钮的第二个按键文字
    int actionId;
    bool hover;
    bool pressed;
    int hoverSub;               // 方向按钮: -1=左/下, 0=无, 1=右/上
};

// ============================================================
// Panel 类
// ============================================================
class Panel
{
public:
    Panel();
    ~Panel();

    // ---- 初始化 ----
    void Init();                    // 创建所有按钮
    void InitGdiPlus(ULONG_PTR& token);  // 初始化 GDI+

    // ---- 绘制 ----
    void Draw(HDC hdc, RECT& panelRect,
              bool isWireframe, bool isOrthoProjection,
              int subdivisionLevel, int algorithmType,
              unsigned int vertexCount, unsigned int faceCount);

    // ---- 交互 ----
    int  HitTest(int relX, int relY) const;           // 返回按钮索引，-1 表示未命中
    int  HitTestDirSub(int btnIdx, int relX, int relY) const; // 方向按钮的子区域
    const GButton& GetButton(int idx) const { return m_buttons[idx]; }
    int  GetButtonCount() const { return m_btnCount; }

    // ---- 按钮状态修改 ----
    void SetButtonHover(int idx, bool hover, int sub = 0);
    void SetButtonPressed(int idx, bool pressed);
    void ClearAllHover();

    // ---- 滚动 ----
    void Scroll(int delta);
    int  GetScrollOffset() const { return m_scrollOffset; }
    int  GetContentHeight() const { return m_contentHeight; }

    // ---- 强制刷新 ----
    void MarkDirty() { m_dirty = true; }

private:
    GButton m_buttons[64];
    int     m_btnCount;
    int     m_scrollOffset;
    int     m_contentHeight;

    // GDI+ 缓存
    bool    m_dirty;

    // ---- 内部辅助 ----
    void AddButton(BtnType type, const wchar_t* label,
                   const wchar_t* key, const wchar_t* key2,
                   int actionId, int y, int h = BTN_HEIGHT);
    void DrawRoundRect(Gdiplus::Graphics& g, Gdiplus::Pen* pen, Gdiplus::Brush* brush,
                       int x, int y, int w, int h, int r);
    Gdiplus::Color ToGdiColor(COLORREF c, BYTE a = 255);

};

#endif // Panel_H
