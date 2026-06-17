// ============================================================
// Panel.cpp - GDI+ 右侧交互面板实现
// ============================================================
#include "Panel.h"
#include <cstdio>

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

// ============================================================
// 构造 / 析构
// ============================================================
Panel::Panel()
    : m_btnCount(0)
    , m_scrollOffset(0)
    , m_contentHeight(0)
    , m_dirty(true)
{
    memset(m_buttons, 0, sizeof(m_buttons));
}

Panel::~Panel()
{
}

// ============================================================
// ToGdiColor - COLORREF → Gdiplus::Color
// ============================================================
Color Panel::ToGdiColor(COLORREF c, BYTE a)
{
    return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

// ============================================================
// DrawRoundRect - 绘制圆角矩形
// ============================================================
void Panel::DrawRoundRect(Graphics& g, Pen* pen, Brush* brush,
                           int x, int y, int w, int h, int r)
{
    GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(pen, &path);
}

// ============================================================
// AddButton - 向按钮列表添加按钮
// ============================================================
void Panel::AddButton(BtnType type, const wchar_t* label,
                       const wchar_t* key, const wchar_t* key2,
                       int actionId, int y, int h)
{
    if (m_btnCount >= 64) return;
    GButton& b = m_buttons[m_btnCount];
    b.type = type;
    b.label = label;
    b.keyText = key;
    b.keyText2 = key2;
    b.actionId = actionId;
    b.hover = false;
    b.pressed = false;
    b.hoverSub = 0;
    SetRect(&b.rect, PANEL_LEFT_PAD, y, PANEL_WIDTH - PANEL_PADDING, y + h);
    m_btnCount++;
}

// ============================================================
// Init - 创建所有按钮（包括细分控制）
// ============================================================
void Panel::Init()
{
    m_btnCount = 0;
    int y = PANEL_PADDING + 8;

    // ---- 标题 ----
    AddButton(BTN_HEADER, L"3D MODEL VIEWER - 细分曲面", L"", L"", 0, y, 36);
    y += 42;

    // ---- 文件 ----
    AddButton(BTN_HEADER, L"▸ 文件", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"保存当前模型", L"Ctrl+S", L"", AID_SAVE_MODEL, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- 显示模式 ----
    AddButton(BTN_HEADER, L"▸ 显示模式", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_TOGGLE, L"线框 / 填充", L"Space", L"", AID_TOGGLE_WIREFRAME, y);
    y += BTN_HEIGHT + 4;
    AddButton(BTN_TOGGLE, L"正交 / 透视投影", L"V", L"", AID_TOGGLE_PROJECTION, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- 细分控制（新增）----
    AddButton(BTN_HEADER, L"▸ 细分控制", L"", L"", 0, y, 26);
    y += 30;

    // 算法选择 — 两个并排按钮，当前选中的高亮
    int algoBtnW = (PANEL_WIDTH - PANEL_LEFT_PAD - PANEL_PADDING - 6) / 2;
    int algoBtnX1 = PANEL_LEFT_PAD;
    int algoBtnX2 = PANEL_LEFT_PAD + algoBtnW + 6;
    AddButton(BTN_ACTION, L"Loop", L"", L"", AID_SUBDIV_ALGO_LOOP, y, BTN_HEIGHT);
    // 手动调整第一个按钮宽度
    {
        GButton& b = m_buttons[m_btnCount - 1];
        SetRect(&b.rect, algoBtnX1, b.rect.top, algoBtnX1 + algoBtnW, b.rect.bottom);
    }
    AddButton(BTN_ACTION, L"Catmull-Clark", L"M", L"", AID_SUBDIV_ALGO_CATMULL, y, BTN_HEIGHT);
    {
        GButton& b = m_buttons[m_btnCount - 1];
        SetRect(&b.rect, algoBtnX2, b.rect.top, algoBtnX2 + algoBtnW, b.rect.bottom);
    }
    y += BTN_HEIGHT + 6;

    // ---- 高级算法分隔 ----
    AddButton(BTN_INFO, L"● 高级算法", L"", L"", 0, y, 22);
    y += 26;
    AddButton(BTN_ACTION, L"Butterfly (插值型)", L"B", L"", AID_SUBDIV_ALGO_BUTTERFLY, y, BTN_HEIGHT);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // 细分级别按钮组 (0-4)
    AddButton(BTN_ACTION, L"级别 0 - 原始模型", L"0", L"", AID_SUBDIV_LEVEL_0, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + 3;
    AddButton(BTN_ACTION, L"级别 1 - 细分 x1", L"1", L"", AID_SUBDIV_LEVEL_1, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + 3;
    AddButton(BTN_ACTION, L"级别 2 - 细分 x2", L"2", L"", AID_SUBDIV_LEVEL_2, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + 3;
    AddButton(BTN_ACTION, L"级别 3 - 细分 x3", L"3", L"", AID_SUBDIV_LEVEL_3, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + 3;
    AddButton(BTN_ACTION, L"级别 4 - 细分 x4", L"4", L"", AID_SUBDIV_LEVEL_4, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + 4;

    // 级别增减
    AddButton(BTN_DIRECTION, L"级别增减", L"[", L"]", AID_SUBDIV_DECR_LEVEL, y, BTN_SMALL_HEIGHT);
    y += BTN_SMALL_HEIGHT + SECTION_GAP + 4;

    // ---- 操作 ----
    AddButton(BTN_HEADER, L"▸ 操作", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"重置视角", L"S", L"", AID_RESET_ALL, y);
    y += BTN_HEIGHT + 4;
    AddButton(BTN_ACTION, L"退出程序", L"Q", L"", AID_QUIT, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- 鼠标操作说明 ----
    AddButton(BTN_HEADER, L"▸ 鼠标操作", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_INFO, L"左键拖拽  —  旋转视角", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"右键拖拽  —  平移视角", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"滚轮  —  缩放", L"", L"", 0, y, 28);
    y += 28 + PANEL_PADDING;

    m_contentHeight = y;
}

// ============================================================
// Draw - 绘制整个面板
// ============================================================
void Panel::Draw(HDC hdc, RECT& panelRect,
                  bool isWireframe, bool isOrthoProjection,
                  int subdivisionLevel, int algorithmType,
                  unsigned int vertexCount, unsigned int faceCount)
{
    int pw = panelRect.right - panelRect.left;
    int ph = panelRect.bottom - panelRect.top;

    // 创建后台缓冲位图（无闪烁）
    Bitmap backBuffer(pw, ph);
    Graphics g(&backBuffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // ---- 面板背景 ----
    SolidBrush bgBrush(ToGdiColor(CLR_BG_PANEL));
    g.FillRectangle(&bgBrush, 0, 0, pw, ph);

    // ---- 左边框线 ----
    Pen borderPen(ToGdiColor(CLR_BORDER), 1.0f);
    g.DrawLine(&borderPen, 0, 0, 0, ph);

    // ---- 标题栏 ----
    SolidBrush titleBg(ToGdiColor(CLR_BG_HEADER));
    g.FillRectangle(&titleBg, 0, 0, pw, 50);

    // ---- 标题下方分隔线 ----
    Pen sepPen(ToGdiColor(CLR_SEPARATOR), 1.0f);
    g.DrawLine(&sepPen, 0, 50, pw, 50);

    // ---- 字体 ----
    Font sectionFont(L"Segoe UI", 9, FontStyleBold);
    Font btnFont(L"Segoe UI", 9);
    Font keyFont(L"Segoe UI", 9, FontStyleBold);
    Font keySmallFont(L"Segoe UI", 8, FontStyleBold);

    SolidBrush sectionBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush btnTextBrush(ToGdiColor(CLR_TEXT));
    SolidBrush keyBrush(ToGdiColor(CLR_TEXT_HEADER));
    SolidBrush keyOffBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush toggleOnBrush(ToGdiColor(CLR_TOGGLE_ON));
    SolidBrush toggleOffBrush(ToGdiColor(CLR_TOGGLE_OFF));

    Pen btnBorder(ToGdiColor(CLR_BORDER), 1.0f);

    // ---- 遍历按钮 ----
    for (int i = 0; i < m_btnCount; i++) {
        GButton& b = m_buttons[i];
        int bx = b.rect.left;
        int by = b.rect.top - m_scrollOffset;
        int bw = b.rect.right - b.rect.left;
        int bh = b.rect.bottom - b.rect.top;

        // 跳过不可见区域
        if (by + bh < 50 || by > ph) continue;

        if (b.type == BTN_HEADER) {
            g.DrawString(b.label, -1, &sectionFont,
                PointF((float)bx + 2, (float)by + 4), &sectionBrush);
            continue;
        }

        if (b.type == BTN_INFO) {
            // 静态信息文本（如鼠标操作说明）
            g.DrawString(b.label, -1, &btnFont,
                PointF((float)bx + 2, (float)by + 4), &sectionBrush);
            continue;
        }

        // 按钮背景
        Color btnBgClr = b.hover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_BUTTON);
        if (b.pressed) btnBgClr = ToGdiColor(CLR_BG_BUTTON_ACTIVE);
        SolidBrush btnBg(btnBgClr);
        DrawRoundRect(g, &btnBorder, &btnBg, bx, by, bw, bh, 5);

        int textX = bx + 10;
        int textY = by + (bh - 16) / 2;

        if (b.type == BTN_TOGGLE) {
            // 标签
            g.DrawString(b.label, -1, &btnFont,
                PointF((float)textX, (float)textY), &btnTextBrush);

            // 计算开关状态
            bool state = false;
            if (b.actionId == AID_TOGGLE_WIREFRAME) state = !isWireframe;
            else if (b.actionId == AID_TOGGLE_PROJECTION) state = !isOrthoProjection;
            else if (b.actionId == AID_SUBDIV_ALGORITHM) state = (algorithmType != 0);

            // 状态指示器
            int indX = bx + bw - 55;
            int indY = by + (bh - 14) / 2;
            SolidBrush* indBrush = state ? &toggleOnBrush : &toggleOffBrush;
            DrawRoundRect(g, NULL, indBrush, indX, indY, 44, 14, 7);
            Font indFont(L"Segoe UI", 7, FontStyleBold);
            SolidBrush indText(Color(255, 255, 255, 255));
            g.DrawString(state ? L"ON" : L"OFF", -1, &indFont,
                PointF((float)indX + (state ? 14 : 10), (float)indY + 1), &indText);

            // 快捷键标记
            int kx = bx + bw - 92;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            g.DrawString(b.keyText, -1, &keySmallFont,
                PointF((float)kx + 6, (float)ky + 3), &keyBrush);

        } else if (b.type == BTN_DIRECTION) {
            // 标签
            g.DrawString(b.label, -1, &btnFont,
                PointF((float)textX, (float)textY), &btnTextBrush);

            // 两个方向按钮
            int dirBtnW = 36;
            int dirBtnH = bh - 6;
            int dirBtnY = by + 3;

            // 左/减按钮
            int negX = bx + bw - dirBtnW * 2 - 16;
            bool negHover = (b.hover && b.hoverSub == -1);
            Color negClr = negHover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_KEY);
            SolidBrush negBg(negClr);
            DrawRoundRect(g, NULL, &negBg, negX, dirBtnY, dirBtnW, dirBtnH, 4);
            g.DrawString(b.keyText2, -1, &keyFont,
                PointF((float)negX + 10, (float)dirBtnY + (dirBtnH - 16) / 2), &keyOffBrush);

            // 右/加按钮
            int posX = negX + dirBtnW + 4;
            bool posHover = (b.hover && b.hoverSub == 1);
            Color posClr = posHover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_KEY);
            SolidBrush posBg(posClr);
            DrawRoundRect(g, NULL, &posBg, posX, dirBtnY, dirBtnW, dirBtnH, 4);
            g.DrawString(b.keyText, -1, &keyFont,
                PointF((float)posX + 10, (float)dirBtnY + (dirBtnH - 16) / 2), &keyBrush);

        } else if (b.type == BTN_ACTION) {
            // 标签
            g.DrawString(b.label, -1, &btnFont,
                PointF((float)textX, (float)textY), &btnTextBrush);

            // 高亮当前级别按钮
            bool isActiveLevel = false;
            if (b.actionId >= AID_SUBDIV_LEVEL_0 && b.actionId <= AID_SUBDIV_LEVEL_4) {
                int level = b.actionId - AID_SUBDIV_LEVEL_0;
                if (level == subdivisionLevel) isActiveLevel = true;
            }
            if (isActiveLevel) {
                Pen activePen(ToGdiColor(CLR_TEXT_HEADER), 2.0f);
                DrawRoundRect(g, &activePen, NULL, bx, by, bw, bh, 5);
            }

            // 高亮当前算法按钮
            bool isActiveAlgo = false;
            if (b.actionId == AID_SUBDIV_ALGO_LOOP && algorithmType == 0) isActiveAlgo = true;
            if (b.actionId == AID_SUBDIV_ALGO_CATMULL && algorithmType == 1) isActiveAlgo = true;
            if (b.actionId == AID_SUBDIV_ALGO_BUTTERFLY && algorithmType == 2) isActiveAlgo = true;
            if (isActiveAlgo) {
                Pen algoPen(ToGdiColor(CLR_TEXT_ACCENT), 2.0f);
                DrawRoundRect(g, &algoPen, NULL, bx, by, bw, bh, 5);
            }

            // 快捷键标记
            int kx = bx + bw - 46;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            SolidBrush quitKeyBrush(Color(255, 220, 100, 100));
            SolidBrush* kptr = (b.actionId == AID_QUIT) ? &quitKeyBrush : &keyBrush;
            g.DrawString(b.keyText, -1, &keySmallFont,
                PointF((float)kx + 8, (float)ky + 3), kptr);
        }
    }

    // 贴回前台
    Graphics screen(hdc);
    screen.DrawImage(&backBuffer, (INT)panelRect.left, (INT)panelRect.top,
        0, 0, pw, ph, UnitPixel);
}

// ============================================================
// HitTest - 按钮命中测试
// ============================================================
int Panel::HitTest(int relX, int relY) const
{
    for (int i = 0; i < m_btnCount; i++) {
        const GButton& b = m_buttons[i];
        if (b.type == BTN_HEADER || b.type == BTN_INFO) continue;
        RECT r = b.rect;
        r.top -= m_scrollOffset;
        r.bottom -= m_scrollOffset;
        if (relX >= r.left && relX <= r.right && relY >= r.top && relY <= r.bottom) {
            return i;
        }
    }
    return -1;
}

// ============================================================
// HitTestDirSub - 方向按钮子区域测试
// ============================================================
int Panel::HitTestDirSub(int btnIdx, int relX, int relY) const
{
    if (btnIdx < 0 || btnIdx >= m_btnCount) return 0;
    const GButton& b = m_buttons[btnIdx];
    if (b.type != BTN_DIRECTION) return 0;

    int bw = b.rect.right - b.rect.left;
    int dirBtnW = 36;
    int negX = b.rect.left + bw - dirBtnW * 2 - 16;
    int posX = negX + dirBtnW + 4;
    int dirBtnY = b.rect.top - m_scrollOffset + 3;
    int dirBtnH = (b.rect.bottom - b.rect.top) - 6;

    if (relX >= negX && relX <= negX + dirBtnW &&
        relY >= dirBtnY && relY <= dirBtnY + dirBtnH)
        return -1;
    if (relX >= posX && relX <= posX + dirBtnW &&
        relY >= dirBtnY && relY <= dirBtnY + dirBtnH)
        return 1;
    return 0;
}

// ============================================================
// SetButtonHover - 设置按钮 hover 状态
// ============================================================
void Panel::SetButtonHover(int idx, bool hover, int sub)
{
    if (idx >= 0 && idx < m_btnCount) {
        m_buttons[idx].hover = hover;
        m_buttons[idx].hoverSub = sub;
    }
}

// ============================================================
// SetButtonPressed - 设置按钮 pressed 状态
// ============================================================
void Panel::SetButtonPressed(int idx, bool pressed)
{
    if (idx >= 0 && idx < m_btnCount) {
        m_buttons[idx].pressed = pressed;
    }
}

// ============================================================
// ClearAllHover - 清除所有按钮 hover 状态
// ============================================================
void Panel::ClearAllHover()
{
    for (int i = 0; i < m_btnCount; i++) {
        m_buttons[i].hover = false;
        m_buttons[i].hoverSub = 0;
    }
}

// ============================================================
// Scroll - 面板滚动
// ============================================================
void Panel::Scroll(int delta)
{
    m_scrollOffset -= delta * 20;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    int maxScroll = m_contentHeight - 600 + 60;
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (maxScroll < 0) m_scrollOffset = 0;
}
