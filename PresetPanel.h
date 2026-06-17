// ============================================================
// PresetPanel.h - 预置模型选择面板（右侧第三栏）
// 展示 EmbeddedModels.h 中嵌入的所有模型
// 两列按钮布局 + 大小模型分类 + 底部"打开其他"按钮
// ============================================================
#ifndef PresetPanel_H
#define PresetPanel_H

#include <windows.h>
#include <gdiplus.h>
#include "Panel.h"  // 复用 GButton, BtnType, ActionId, 颜色常量

// 预置模型动作 ID 起始偏移
#define AID_PRESET_MODEL_BASE  1000

// 预置面板宽度
#define PRESET_PANEL_WIDTH 280

// ---- ModelList.h 访问函数 ----
int             GetModelResId(int idx);
const wchar_t*  GetModelName(int idx);
int             GetModelCount();

class PresetPanel
{
public:
    PresetPanel();
    ~PresetPanel();

    // ---- 初始化 ----
    void Init();    // 从 g_embeddedModels 创建按钮

    // ---- 绘制 ----
    void Draw(HDC hdc, RECT& panelRect);

    // ---- 交互 ----
    int  HitTest(int relX, int relY) const;
    const GButton& GetButton(int idx) const { return m_buttons[idx]; }
    int  GetButtonCount() const { return m_btnCount; }

    // ---- 按钮状态 ----
    void SetButtonHover(int idx, bool hover, int sub = 0);
    void SetButtonPressed(int idx, bool pressed);
    void ClearAllHover();

    // ---- 滚动 ----
    void Scroll(int delta);
    int  GetScrollOffset() const { return m_scrollOffset; }
    int  GetContentHeight() const { return m_contentHeight; }

private:
    GButton m_buttons[64];
    int     m_btnCount;
    int     m_scrollOffset;
    int     m_contentHeight;

    // ---- GDI+ 辅助 ----
    void DrawRoundRect(Gdiplus::Graphics& g, Gdiplus::Pen* pen, Gdiplus::Brush* brush,
                       int x, int y, int w, int h, int r);
    Gdiplus::Color ToGdiColor(COLORREF c, BYTE a = 255);
};

#endif // PresetPanel_H
