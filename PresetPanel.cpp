// ============================================================
// PresetPanel.cpp - 预置模型选择面板实现
// ============================================================
#include "PresetPanel.h"
#include "ModelList.h"
#include <cstdio>

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

// ---- 局部常量 ----
#define PADDING      8
#define BTN_GAP_H    6     // 两按钮水平间距
#define PRESET_BTN_H   56    // 模型按钮高度
#define OPEN_BTN_H   34    // "打开其他" 按钮高度

PresetPanel::PresetPanel()
    : m_btnCount(0), m_scrollOffset(0), m_contentHeight(0)
{
    memset(m_buttons, 0, sizeof(m_buttons));
}

PresetPanel::~PresetPanel() {}

// ============================================================
// GDI+ 辅助
// ============================================================
Color PresetPanel::ToGdiColor(COLORREF c, BYTE a) {
    return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

void PresetPanel::DrawRoundRect(Graphics& g, Pen* pen, Brush* brush,
                                 int x, int y, int w, int h, int r) {
    GraphicsPath path;
    path.AddArc(x, y, r*2, r*2, 180, 90);
    path.AddArc(x+w-r*2, y, r*2, r*2, 270, 90);
    path.AddArc(x+w-r*2, y+h-r*2, r*2, r*2, 0, 90);
    path.AddArc(x, y+h-r*2, r*2, r*2, 90, 90);
    path.CloseFigure();
    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(pen, &path);
}

// ============================================================
// Init - 从 g_modelList 创建按钮布局
// ============================================================
void PresetPanel::Init()
{
    m_btnCount = 0;
    int y = PADDING;
    int btnW = (PRESET_PANEL_WIDTH - 2*PADDING - BTN_GAP_H) / 2;

    // ---- 标题 ----
    GButton& hdr = m_buttons[m_btnCount++];
    hdr.type = BTN_HEADER;
    hdr.label = L"预置模型";
    SetRect(&hdr.rect, PADDING, y, PRESET_PANEL_WIDTH - PADDING, y + 28);
    y += 32;

    // ---- 分隔：小模型 ----
    GButton& secSmall = m_buttons[m_btnCount++];
    secSmall.type = BTN_INFO;
    secSmall.label = L"● 小模型 (< 500 KB)";
    SetRect(&secSmall.rect, PADDING, y, PRESET_PANEL_WIDTH - PADDING, y + 20);
    y += 24;

    // 添加小模型按钮（两列）
    int smallCount = 0;
    for (int i = 0; i < g_modelListCount; i++) {
        if (g_modelList[i].isLarge) continue;

        int col = smallCount % 2;
        int bx = PADDING + col * (btnW + BTN_GAP_H);
        int by = y + (smallCount / 2) * (PRESET_BTN_H + 4);

        GButton& btn = m_buttons[m_btnCount++];
        btn.type = BTN_ACTION;
        btn.actionId = AID_PRESET_MODEL_BASE + i;
        btn.label = g_modelList[i].name;
        SetRect(&btn.rect, bx, by, bx + btnW, by + PRESET_BTN_H);
        smallCount++;
    }
    y += ((smallCount + 1) / 2) * (PRESET_BTN_H + 4) + 4;

    // ---- 分隔：大模型 ----
    GButton& secLarge = m_buttons[m_btnCount++];
    secLarge.type = BTN_INFO;
    secLarge.label = L"● 大模型 (≥ 500 KB)";
    SetRect(&secLarge.rect, PADDING, y, PRESET_PANEL_WIDTH - PADDING, y + 20);
    y += 24;

    // 添加大模型按钮（两列）
    int largeCount = 0;
    for (int i = 0; i < g_modelListCount; i++) {
        if (!g_modelList[i].isLarge) continue;

        int col = largeCount % 2;
        int bx = PADDING + col * (btnW + BTN_GAP_H);
        int by = y + (largeCount / 2) * (PRESET_BTN_H + 4);

        GButton& btn = m_buttons[m_btnCount++];
        btn.type = BTN_ACTION;
        btn.actionId = AID_PRESET_MODEL_BASE + i;
        btn.label = g_modelList[i].name;
        SetRect(&btn.rect, bx, by, bx + btnW, by + PRESET_BTN_H);
        largeCount++;
    }
    y += ((largeCount + 1) / 2) * (PRESET_BTN_H + 4) + 8;

    // ---- 分隔线 ----
    GButton& sep = m_buttons[m_btnCount++];
    sep.type = BTN_INFO;
    sep.label = L"── 其他 ──";
    SetRect(&sep.rect, PADDING, y, PRESET_PANEL_WIDTH - PADDING, y + 20);
    y += 24;

    // ---- 打开其他 OBJ 按钮（藏在最下面）----
    GButton& openBtn = m_buttons[m_btnCount++];
    openBtn.type = BTN_ACTION;
    openBtn.label = L"打开其他 OBJ 文件...";
    openBtn.actionId = AID_OPEN_MODEL;
    openBtn.keyText = L"O";
    SetRect(&openBtn.rect, PADDING, y, PRESET_PANEL_WIDTH - PADDING, y + OPEN_BTN_H);
    y += OPEN_BTN_H + PADDING;

    m_contentHeight = y;
}

// ============================================================
// Draw - GDI+ 绘制整个面板
// ============================================================
void PresetPanel::Draw(HDC hdc, RECT& panelRect)
{
    int pw = panelRect.right - panelRect.left;
    int ph = panelRect.bottom - panelRect.top;
    if (pw <= 0 || ph <= 0) return;

    Bitmap backBuffer(pw, ph);
    Graphics g(&backBuffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // ---- 背景 ----
    SolidBrush bgBrush(ToGdiColor(CLR_BG_PANEL));
    g.FillRectangle(&bgBrush, 0, 0, pw, ph);

    // ---- 左边框线 ----
    Pen borderPen(ToGdiColor(CLR_BORDER), 1.0f);
    g.DrawLine(&borderPen, 0, 0, 0, ph);

    // ---- 顶部标题栏 ----
    SolidBrush titleBg(ToGdiColor(CLR_BG_HEADER));
    g.FillRectangle(&titleBg, 0, 0, pw, 36);
    Pen sepPen(ToGdiColor(CLR_SEPARATOR), 1.0f);
    g.DrawLine(&sepPen, 0, 36, pw, 36);

    // ---- 字体 ----
    Font titleFont(L"Segoe UI", 10, FontStyleBold);
    Font sectionFont(L"Segoe UI", 9, FontStyleBold);
    Font btnNameFont(L"Segoe UI", 10, FontStyleBold);
    Font btnSizeFont(L"Segoe UI", 8);
    Font openFont(L"Segoe UI", 9);

    SolidBrush textBrush(ToGdiColor(CLR_TEXT));
    SolidBrush dimBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush accentBrush(ToGdiColor(CLR_TEXT_ACCENT));
    SolidBrush titleTextBrush(ToGdiColor(CLR_TEXT_HEADER));

    Pen btnBorder(ToGdiColor(CLR_BORDER), 1.0f);

    // ---- 遍历按钮 ----
    for (int i = 0; i < m_btnCount; i++) {
        const GButton& b = m_buttons[i];
        int bx = b.rect.left;
        int by = b.rect.top - m_scrollOffset;
        int bw = b.rect.right - b.rect.left;
        int bh = b.rect.bottom - b.rect.top;

        if (by + bh < 36 || by > ph) continue;

        if (b.type == BTN_HEADER) {
            g.DrawString(b.label, -1, &titleFont,
                PointF((float)bx, (float)by + 4), &titleTextBrush);
            continue;
        }

        if (b.type == BTN_INFO) {
            g.DrawString(b.label, -1, &sectionFont,
                PointF((float)bx, (float)by + 2), &dimBrush);
            continue;
        }

        // ---- BTN_ACTION: 模型按钮或打开按钮 ----
        Color btnBgClr = b.hover ? ToGdiColor(CLR_BG_BUTTON_HOVER)
                                 : ToGdiColor(CLR_BG_BUTTON);
        if (b.pressed) btnBgClr = ToGdiColor(CLR_BG_BUTTON_ACTIVE);
        SolidBrush btnBg(btnBgClr);
        DrawRoundRect(g, &btnBorder, &btnBg, bx, by, bw, bh, 6);

        if (b.actionId == AID_OPEN_MODEL) {
            // "打开其他 OBJ 文件..." — 单行居中
            RectF layoutRect((float)bx, (float)by, (float)bw, (float)bh);
            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(b.label, -1, &openFont, layoutRect, &sf, &textBrush);
        } else if (b.actionId >= AID_PRESET_MODEL_BASE) {
            // 模型按钮：名称 + 大小
            int modelIdx = b.actionId - AID_PRESET_MODEL_BASE;
            unsigned int sizeKB = (modelIdx < g_modelListCount)
                ? g_modelList[modelIdx].sizeKB : 0;

            // 模型名称（居中偏上）
            RectF nameRect((float)bx, (float)by + 6, (float)bw, 22.0f);
            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(b.label, -1, &btnNameFont, nameRect, &sf, &textBrush);

            // 文件大小（下方居中）
            wchar_t sizeStr[32];
            if (sizeKB >= 1024)
                swprintf(sizeStr, 32, L"%.1f MB", sizeKB / 1024.0f);
            else
                swprintf(sizeStr, 32, L"%u KB", sizeKB);
            RectF sizeRect((float)bx, (float)by + 30, (float)bw, 18.0f);
            g.DrawString(sizeStr, -1, &btnSizeFont, sizeRect, &sf, &dimBrush);
        }
    }

    // 贴回前台
    Graphics screen(hdc);
    screen.DrawImage(&backBuffer, (INT)panelRect.left, (INT)panelRect.top,
        0, 0, pw, ph, UnitPixel);
}

// ============================================================
// HitTest
// ============================================================
int PresetPanel::HitTest(int relX, int relY) const
{
    for (int i = 0; i < m_btnCount; i++) {
        if (m_buttons[i].type == BTN_HEADER || m_buttons[i].type == BTN_INFO)
            continue;
        RECT r = m_buttons[i].rect;
        r.top -= m_scrollOffset;
        r.bottom -= m_scrollOffset;
        if (relX >= r.left && relX <= r.right && relY >= r.top && relY <= r.bottom)
            return i;
    }
    return -1;
}

// ============================================================
// 按钮状态
// ============================================================
void PresetPanel::SetButtonHover(int idx, bool hover, int sub) {
    if (idx >= 0 && idx < m_btnCount) {
        m_buttons[idx].hover = hover;
        m_buttons[idx].hoverSub = sub;
    }
}
void PresetPanel::SetButtonPressed(int idx, bool pressed) {
    if (idx >= 0 && idx < m_btnCount)
        m_buttons[idx].pressed = pressed;
}
void PresetPanel::ClearAllHover() {
    for (int i = 0; i < m_btnCount; i++) {
        m_buttons[i].hover = false;
        m_buttons[i].hoverSub = 0;
    }
}

// ============================================================
// Scroll
// ============================================================
void PresetPanel::Scroll(int delta) {
    m_scrollOffset -= delta * 20;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    int maxScroll = m_contentHeight - 600 + 60;
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (maxScroll < 0) m_scrollOffset = 0;
}

// ============================================================
// ModelList.h 访问函数（给 main.cpp 使用）
// ============================================================
int GetModelResId(int idx) {
    return (idx >= 0 && idx < g_modelListCount) ? g_modelList[idx].resId : -1;
}
const wchar_t* GetModelName(int idx) {
    return (idx >= 0 && idx < g_modelListCount) ? g_modelList[idx].name : L"";
}
int GetModelCount() {
    return g_modelListCount;
}
