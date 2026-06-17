// ============================================================
// Subdivision Surface Viewer
// Win32 OpenGL + GDI+ 交互式网格查看器
// 支持 Loop / Catmull-Clark 细分曲面
//
// 重构自原 ModelViewer，模块化为:
//   Mesh.h/cpp      - 网格数据与 OBJ 加载
//   Renderer.h/cpp  - OpenGL 渲染
//   Panel.h/cpp     - GDI+ 交互面板
//   Camera.h/cpp    - 相机
// ============================================================

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <commdlg.h>
#include "Mesh.h"
#include "Renderer.h"
#include "Panel.h"
#include "Camera.h"
#include "SubdivisionLoop.h"
#include "SubdivisionCatmullClark.h"
#include "SubdivisionButterfly.h"
#include "PresetPanel.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>

using namespace Gdiplus;

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// ============================================================
// Forward declarations
// ============================================================
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK OpenGLWndProc(HWND, UINT, WPARAM, LPARAM);
void ExecuteAction(int actionId, int sub);
void SetSubdivisionLevel(int level);
void RebuildSubdivisionMesh();
void LoadOBJFile(const char* filename);
void ActionToggleWireframe();
void ActionToggleProjection();
void ActionOpenModel();
void ActionSaveModel();
void ActionResetAll();
void ActionQuit();
void LoadEmbeddedModel(int modelIdx);

// ============================================================
// Global state
// ============================================================
static HWND     g_hMainWnd = NULL;
static HWND     g_hGLWnd = NULL;
static int      g_glWidth = 0, g_glHeight = 0;

// 核心数据
static Mesh     g_originalMesh;     // 原始加载的网格（不变）
static Mesh     g_displayMesh;      // 当前显示的网格（可能已细分）
static Camera   g_camera;
static Renderer g_renderer;
static Panel       g_panel;
static PresetPanel g_presetPanel;

// 渲染状态
static int   g_isWire = 1;
static int   g_isOrtho = 1;
static bool  g_modelLoaded = false;

// 细分状态
static int              g_subdivisionLevel = 0;    // 0=原始, 1-4=细分级别
static int              g_algorithmType = 0;       // 0=Loop, 1=CatmullClark
static LoopSubdivision          g_loopSubdiv;       // Loop 细分对象
static CatmullClarkSubdivision   g_catmullSubdiv;    // Catmull-Clark 细分对象
static SubdivisionButterfly      g_butterflySubdiv;   // Butterfly 细分对象
static Mesh                     g_triangulatedMesh;  // 三角化后的网格 (Loop/Butterfly 需要)

// 细分结果缓存: cache[algorithm][level] (0..2 × 0..4)
static Mesh g_meshCache[3][5];
static bool g_cacheValid[3][5] = {};

void InvalidateMeshCache() {
    for (int a = 0; a < 3; a++)
        for (int l = 0; l < 5; l++)
            g_cacheValid[a][l] = false;
}

// 鼠标交互状态
static bool  g_mouseLDown = false;
static bool  g_mouseRDown = false;
static int   g_lastMouseX = 0;
static int   g_lastMouseY = 0;

// GDI+
static ULONG_PTR g_gdiToken = 0;

// FPS 跟踪
static int    g_frameCount = 0;
static DWORD  g_lastFpsTime = 0;
static float  g_currentFps = 0.0f;

// Model 变换状态（保留用于兼容）
static float g_modelX = 0, g_modelY = 0, g_modelZ = 0;
static float g_modelRotX = 0, g_modelRotY = 0, g_modelRotZ = 0;

// ============================================================
// ExecuteAction - 执行按钮/键盘操作
// ============================================================
void ExecuteAction(int actionId, int sub)
{
    switch (actionId) {
    case AID_TOGGLE_WIREFRAME:  ActionToggleWireframe(); break;
    case AID_TOGGLE_PROJECTION: ActionToggleProjection(); break;
    case AID_MODEL_TRANS_X_POS: g_modelX += sub * 0.1f; break;
    case AID_MODEL_TRANS_Y_POS: g_modelY += sub * 0.1f; break;
    case AID_MODEL_TRANS_Z_POS: g_modelZ += sub * 0.1f; break;
    case AID_MODEL_ROT_X_POS:
        g_modelRotX += sub * 10;
        if (g_modelRotX < 0) g_modelRotX += 360;
        if (g_modelRotX >= 360) g_modelRotX -= 360;
        break;
    case AID_MODEL_ROT_Y_POS:
        g_modelRotY += sub * 10;
        if (g_modelRotY < 0) g_modelRotY += 360;
        if (g_modelRotY >= 360) g_modelRotY -= 360;
        break;
    case AID_MODEL_ROT_Z_POS:
        g_modelRotZ += sub * 10;
        if (g_modelRotZ < 0) g_modelRotZ += 360;
        if (g_modelRotZ >= 360) g_modelRotZ -= 360;
        break;
    case AID_CAMERA_TRANS_X_POS: g_camera.camera_x += sub * 0.1f; break;
    case AID_CAMERA_TRANS_Y_POS: g_camera.camera_y += sub * 0.1f; break;
    case AID_CAMERA_TRANS_Z_POS:
        g_camera.camera_distance -= sub * 0.15f;
        if (g_camera.camera_distance < 0.5f)  g_camera.camera_distance = 0.5f;
        if (g_camera.camera_distance > 50.0f) g_camera.camera_distance = 50.0f;
        break;
    case AID_CAMERA_ROT_X_POS:
        g_camera.camera_rotx += sub * 10;
        if (g_camera.camera_rotx >= 360) g_camera.camera_rotx -= 360;
        if (g_camera.camera_rotx < 0) g_camera.camera_rotx += 360;
        break;
    case AID_CAMERA_ROT_Y_POS:
        g_camera.camera_roty += sub * 10;
        if (g_camera.camera_roty >= 360) g_camera.camera_roty -= 360;
        if (g_camera.camera_roty < 0) g_camera.camera_roty += 360;
        break;
    case AID_CAMERA_ROT_Z_POS:
        g_camera.camera_rotz += sub * 10;
        if (g_camera.camera_rotz >= 360) g_camera.camera_rotz -= 360;
        if (g_camera.camera_rotz < 0) g_camera.camera_rotz += 360;
        break;

    case AID_SAVE_MODEL:  ActionSaveModel(); return;
    case AID_RESET_ALL:   ActionResetAll(); return;
    case AID_QUIT:        ActionQuit(); return;
    case AID_OPEN_MODEL:  ActionOpenModel(); return;

    default:
        // 预置模型: AID_PRESET_MODEL_BASE + index
        if (actionId >= AID_PRESET_MODEL_BASE) {
            LoadEmbeddedModel(actionId - AID_PRESET_MODEL_BASE);
            return;
        }
        break;

    // ---- 细分控制 ----
    case AID_SUBDIV_LEVEL_0: SetSubdivisionLevel(0); return;
    case AID_SUBDIV_LEVEL_1: SetSubdivisionLevel(1); return;
    case AID_SUBDIV_LEVEL_2: SetSubdivisionLevel(2); return;
    case AID_SUBDIV_LEVEL_3: SetSubdivisionLevel(3); return;
    case AID_SUBDIV_LEVEL_4: SetSubdivisionLevel(4); return;
    case AID_SUBDIV_DECR_LEVEL:
        if (sub == -1 && g_subdivisionLevel > 0) SetSubdivisionLevel(g_subdivisionLevel - 1);
        if (sub == 1  && g_subdivisionLevel < 4) SetSubdivisionLevel(g_subdivisionLevel + 1);
        return;
    case AID_SUBDIV_ALGO_LOOP:
        if (g_algorithmType != 0) { g_algorithmType = 0; RebuildSubdivisionMesh(); }
        return;
    case AID_SUBDIV_ALGO_CATMULL:
        if (g_algorithmType != 1) { g_algorithmType = 1; RebuildSubdivisionMesh(); }
        return;
    case AID_SUBDIV_ALGO_BUTTERFLY:
        if (g_algorithmType != 2) { g_algorithmType = 2; RebuildSubdivisionMesh(); }
        return;
    case AID_SUBDIV_ALGORITHM:
        g_algorithmType = (g_algorithmType + 1) % 3;  // Loop→Catmull-Clark→Butterfly→Loop
        RebuildSubdivisionMesh();
        return;
    }

    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

// ============================================================
// 细分相关函数（Phase 2 将实现具体算法）
// ============================================================
void SetSubdivisionLevel(int level)
{
    if (!g_modelLoaded) return;
    if (level < 0 || level > 4) return;
    g_subdivisionLevel = level;
    RebuildSubdivisionMesh();
}

void RebuildSubdivisionMesh()
{
    if (!g_modelLoaded) return;

    int algo = g_algorithmType;
    int target = g_subdivisionLevel;

    if (target == 0) {
        g_displayMesh = g_originalMesh;
    }
    else if (g_cacheValid[algo][target]) {
        // ---- 命中缓存 ----
        g_displayMesh = g_meshCache[algo][target];
    }
    else {
        // ---- 未命中：找最高已缓存级别，逐级算 ----
        int startLevel = 0;
        Mesh stepMesh;

        for (int l = target - 1; l >= 1; l--) {
            if (g_cacheValid[algo][l]) {
                startLevel = l;
                stepMesh = g_meshCache[algo][l];
                break;
            }
        }

        if (startLevel == 0) {
            // 无缓存，准备起始网格
            if (algo == 0 || algo == 2) {
                // Loop / Butterfly: 需要三角化
                stepMesh = g_originalMesh;
                stepMesh.Triangulate();
            } else {
                // Catmull-Clark: 直接用原始网格
                stepMesh = g_originalMesh;
            }
        }

        // 逐级细分，沿途缓存
        bool ok = true;
        for (int lev = startLevel + 1; lev <= target; lev++) {
            bool success = false;
            Mesh outMesh;

            switch (algo) {
            case 0: // Loop
                if (g_loopSubdiv.LoadFromMesh(stepMesh)) {
                    g_loopSubdiv.SubdivideOnce();
                    outMesh = g_loopSubdiv.ToMesh();
                    success = true;
                }
                break;
            case 1: // Catmull-Clark
                if (g_catmullSubdiv.LoadFromMesh(stepMesh)) {
                    g_catmullSubdiv.SubdivideOnce();
                    outMesh = g_catmullSubdiv.ToMesh();
                    success = true;
                }
                break;
            case 2: // Butterfly
                if (g_butterflySubdiv.LoadFromMesh(stepMesh)) {
                    g_butterflySubdiv.SubdivideOnce();
                    outMesh = g_butterflySubdiv.ToMesh();
                    success = true;
                }
                break;
            }

            if (!success) { ok = false; break; }

            g_meshCache[algo][lev] = outMesh;
            g_cacheValid[algo][lev] = true;
            stepMesh = outMesh;
        }

        g_displayMesh = ok ? stepMesh : g_originalMesh;
    }

    // 重建 display list
    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_renderer.GetGLRC());
    g_renderer.BuildDisplayList(g_displayMesh);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);

    InvalidateRect(g_hGLWnd, NULL, FALSE);
    RECT r;
    GetClientRect(g_hMainWnd, &r);
    r.left = g_glWidth;
    InvalidateRect(g_hMainWnd, &r, FALSE);
}

// ============================================================
// Action 实现
// ============================================================
void ActionToggleWireframe()
{
    g_isWire = !g_isWire;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionToggleProjection()
{
    g_isOrtho = !g_isOrtho;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionSaveModel()
{
    if (g_modelLoaded) {
        g_displayMesh.SaveToOBJ("output.obj");
        MessageBox(g_hMainWnd, L"模型已保存到 output.obj", L"保存成功", MB_OK | MB_ICONINFORMATION);
    }
}

void ActionResetAll()
{
    g_modelX = g_modelY = g_modelZ = 0;
    g_modelRotX = g_modelRotY = g_modelRotZ = 0;
    g_camera.resetVars();
    g_camera.camera_distance = 3.0f;
    g_isWire = 1;
    g_isOrtho = 1;
    g_subdivisionLevel = 0;
    if (g_modelLoaded) {
        g_displayMesh = g_originalMesh;
        HDC hdc = GetDC(g_hGLWnd);
        wglMakeCurrent(hdc, g_renderer.GetGLRC());
        g_renderer.BuildDisplayList(g_displayMesh);
        wglMakeCurrent(NULL, NULL);
        ReleaseDC(g_hGLWnd, hdc);
    }
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionQuit()
{
    PostQuitMessage(0);
}

void ActionOpenModel()
{
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Wavefront OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"打开 Wavefront OBJ 模型文件";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        char narrowPath[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, narrowPath, MAX_PATH, NULL, NULL);

        // 重置状态
        ActionResetAll();
        g_modelLoaded = false;
        g_originalMesh.Clear();
        g_displayMesh.Clear();

        // 加载
        LoadOBJFile(narrowPath);

        InvalidateRect(g_hGLWnd, NULL, FALSE);
        UpdateWindow(g_hGLWnd);
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        UpdateWindow(g_hMainWnd);
    }
}

// ============================================================
// LoadOBJFile - 加载 OBJ 文件并构建 display list
// ============================================================
void LoadOBJFile(const char* filename)
{
    if (!g_originalMesh.LoadFromOBJ(filename)) {
        wchar_t buf[512];
        swprintf(buf, 512, L"无法打开文件:\n%hs", filename);
        MessageBox(NULL, buf, L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    g_modelLoaded = true;
    g_displayMesh = g_originalMesh;

    InvalidateMeshCache();

    // 构建 display list
    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_renderer.GetGLRC());
    g_renderer.BuildDisplayList(g_displayMesh);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
}

// ============================================================
// LoadEmbeddedModel - 从嵌入数据加载预置模型
// ============================================================
void LoadEmbeddedModel(int modelIdx)
{
    if (modelIdx < 0 || modelIdx >= GetModelCount()) return;

    int resId = GetModelResId(modelIdx);
    if (resId < 0) return;

    g_modelLoaded = false;
    g_originalMesh.Clear();
    g_displayMesh.Clear();
    g_subdivisionLevel = 0;

    // 确保相机有合理的初始距离（全局静态变量零初始化后 camera_distance=0）
    if (g_camera.camera_distance < 0.1f)
        g_camera.camera_distance = 3.0f;

    // 从 exe 内嵌资源加载模型
    HINSTANCE hInst = GetModuleHandle(NULL);
    if (!g_originalMesh.LoadFromResource(hInst, resId)) {
        wchar_t buf[256];
        swprintf(buf, 256, L"无法加载嵌入模型: %s", GetModelName(modelIdx));
        MessageBox(NULL, buf, L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    g_modelLoaded = true;
    g_displayMesh = g_originalMesh;

    InvalidateMeshCache();

    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_renderer.GetGLRC());
    g_renderer.BuildDisplayList(g_displayMesh);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);

    InvalidateRect(g_hGLWnd, NULL, FALSE);
    UpdateWindow(g_hGLWnd);
    InvalidateRect(g_hMainWnd, NULL, FALSE);
    UpdateWindow(g_hMainWnd);
}

// ============================================================
// DrawInfoOverlay - 在 OpenGL 视图顶部居中绘制模型信息
// 使用 GDI 在 SwapBuffers 之后绘制，叠加在 3D 视图上方
// ============================================================
void DrawInfoOverlay(HDC hdc, int width, int height)
{
    if (!g_modelLoaded) return;

    // 构建信息文本
    const wchar_t* algoNames[] = { L"Loop", L"Catmull-Clark", L"Butterfly" };
    const wchar_t* algoName = algoNames[g_algorithmType % 3];
    const wchar_t* colorName = (g_subdivisionLevel == 0) ? L"原始(蓝)" : L"细分(金)";
    wchar_t infoText[512];
    swprintf(infoText, 512,
        L"算法: %s  |  级别: %d  |  顶点: %u  |  面: %u  |  颜色: %s  |  FPS: %.1f",
        algoName, g_subdivisionLevel,
        g_displayMesh.VertexCount(), g_displayMesh.FaceCount(),
        colorName, g_currentFps);

    // 计算文本尺寸和位置
    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    RECT textRect = { 0, 6, width, 40 };
    // 先测量文本实际宽度，以便画出适配的底色条
    RECT measureRect = textRect;
    DrawTextW(hdc, infoText, -1, &measureRect, DT_CALCRECT | DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    int textW = measureRect.right - measureRect.left;

    // 底色条：半透明深色背景，比文本略宽
    int barW = textW + 40;
    int barH = 32;
    int barX = (width - barW) / 2;
    int barY = 8;
    if (barX < 0) barX = 4;

    // 用 GDI 画圆角矩形背景
    HBRUSH hBgBrush = CreateSolidBrush(RGB(20, 20, 25));
    HPEN hBgPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 70));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBgBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBgPen);
    RoundRect(hdc, barX, barY, barX + barW, barY + barH, 12, 12);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBgBrush);
    DeleteObject(hBgPen);

    // 绘制文本
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(210, 210, 215));
    RECT drawRect = { barX, barY, barX + barW, barY + barH };
    DrawTextW(hdc, infoText, -1, &drawRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

// ============================================================
// OpenGL 子窗口过程
// ============================================================
LRESULT CALLBACK OpenGLWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        g_renderer.Initialize(hWnd);
        return 0;

    case WM_SIZE:
        g_renderer.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        wglMakeCurrent(hdc, g_renderer.GetGLRC());

        if (g_modelLoaded) {
            g_renderer.Render(g_displayMesh,
                g_camera.camera_distance,
                g_camera.camera_rotx, g_camera.camera_roty,
                g_camera.camera_x, g_camera.camera_y,
                g_modelX, g_modelY, g_modelZ,
                g_modelRotX, g_modelRotY, g_modelRotZ,
                g_isWire != 0, g_isOrtho != 0,
                g_subdivisionLevel);
        } else {
            glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        SwapBuffers(hdc);

        // 在 3D 视图上方绘制模型信息覆盖层
        DrawInfoOverlay(hdc, g_glWidth, g_glHeight);

        wglMakeCurrent(NULL, NULL);
        EndPaint(hWnd, &ps);

        // FPS 计算
        g_frameCount++;
        DWORD now = GetTickCount();
        if (g_lastFpsTime == 0) g_lastFpsTime = now;
        DWORD elapsed = now - g_lastFpsTime;
        if (elapsed >= 1000) {
            g_currentFps = g_frameCount * 1000.0f / elapsed;
            g_frameCount = 0;
            g_lastFpsTime = now;
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
        SetCapture(hWnd);
        g_mouseLDown = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        return 0;

    case WM_LBUTTONUP:
        ReleaseCapture();
        g_mouseLDown = false;
        return 0;

    case WM_RBUTTONDOWN:
        SetCapture(hWnd);
        g_mouseRDown = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
        g_mouseRDown = false;
        return 0;

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int dx = mx - g_lastMouseX;
        int dy = my - g_lastMouseY;
        g_lastMouseX = mx;
        g_lastMouseY = my;

        if (g_mouseLDown) {
            g_camera.camera_roty -= dx * 0.3f;
            g_camera.camera_rotx += dy * 0.3f;
            if (g_camera.camera_rotx > 85.0f)  g_camera.camera_rotx = 85.0f;
            if (g_camera.camera_rotx < -85.0f) g_camera.camera_rotx = -85.0f;
            if (g_camera.camera_roty > 360.0f)  g_camera.camera_roty -= 360.0f;
            if (g_camera.camera_roty < 0.0f)    g_camera.camera_roty += 360.0f;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (g_mouseRDown) {
            float sens = 0.001f * g_camera.camera_distance;
            float rx = g_camera.camera_rotx * 3.14159f / 180.0f;
            float ry = g_camera.camera_roty * 3.14159f / 180.0f;

            float rX =  cosf(ry);
            float rZ = -sinf(ry);
            float uX = -sinf(ry) * sinf(rx);
            float uY =  cosf(rx);
            float uZ = -cosf(ry) * sinf(rx);

            float sdx = -dx * sens;
            float sdy =  dy * sens;

            g_modelX += sdx * rX + sdy * uX;
            g_modelY += sdy * uY;
            g_modelZ += sdx * rZ + sdy * uZ;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        g_camera.camera_distance -= delta * 0.3f;
        if (g_camera.camera_distance < 0.5f)  g_camera.camera_distance = 0.5f;
        if (g_camera.camera_distance > 50.0f) g_camera.camera_distance = 50.0f;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        g_renderer.Shutdown();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
// 主窗口过程
// ============================================================
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        g_hMainWnd = hWnd;

        // 创建 OpenGL 子窗口
        WNDCLASS wcGL = {};
        wcGL.lpfnWndProc = OpenGLWndProc;
        wcGL.hInstance = GetModuleHandle(NULL);
        wcGL.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcGL.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcGL.lpszClassName = L"OpenGLWindow";
        RegisterClass(&wcGL);

        g_hGLWnd = CreateWindow(L"OpenGLWindow", NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 100, 100, hWnd, NULL, GetModuleHandle(NULL), NULL);

        // 初始化 GDI+
        GdiplusStartupInput gdiStartupInput;
        GdiplusStartup(&g_gdiToken, &gdiStartupInput, NULL);

        // 初始化面板
        g_panel.Init();
        g_presetPanel.Init();
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        int glw = w - PANEL_WIDTH - PRESET_PANEL_WIDTH;
        if (glw < 200) glw = 200;
        g_glWidth = glw;
        g_glHeight = h;
        MoveWindow(g_hGLWnd, 0, 0, glw, h, TRUE);

        // 使两个面板区域无效
        RECT r;
        r.left = glw;
        r.top = 0;
        r.right = w;
        r.bottom = h;
        InvalidateRect(hWnd, &r, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        int w = clientRect.right;
        int h = clientRect.bottom;

        // 中栏：属性面板
        RECT propRect;
        propRect.left   = w - PANEL_WIDTH - PRESET_PANEL_WIDTH;
        propRect.top    = 0;
        propRect.right  = w - PRESET_PANEL_WIDTH;
        propRect.bottom = h;

        // 右栏：预置模型面板
        RECT presetRect;
        presetRect.left   = w - PRESET_PANEL_WIDTH;
        presetRect.top    = 0;
        presetRect.right  = w;
        presetRect.bottom = h;

        g_panel.Draw(hdc, propRect,
            g_isWire != 0, g_isOrtho != 0,
            g_subdivisionLevel, g_algorithmType,
            g_displayMesh.VertexCount(), g_displayMesh.FaceCount());

        g_presetPanel.Draw(hdc, presetRect);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int glw = g_glWidth;

        if (mx < glw) break; // OpenGL 区域

        int zoneX = mx - glw;

        if (zoneX < PANEL_WIDTH) {
            // ---- 中栏：属性面板 ----
            int btnIdx = g_panel.HitTest(zoneX, my);
            for (int i = 0; i < g_panel.GetButtonCount(); i++) {
                if (i == btnIdx) {
                    int sub = g_panel.HitTestDirSub(i, zoneX, my);
                    if (!g_panel.GetButton(i).hover || g_panel.GetButton(i).hoverSub != sub) {
                        g_panel.SetButtonHover(i, true, sub);
                        RECT r2; GetClientRect(hWnd, &r2);
                        r2.left = glw; r2.right = glw + PANEL_WIDTH;
                        InvalidateRect(hWnd, &r2, FALSE);
                    }
                } else if (g_panel.GetButton(i).hover) {
                    g_panel.SetButtonHover(i, false);
                    RECT r2; GetClientRect(hWnd, &r2);
                    r2.left = glw; r2.right = glw + PANEL_WIDTH;
                    InvalidateRect(hWnd, &r2, FALSE);
                }
            }
        } else {
            // ---- 右栏：预置模型面板 ----
            int relX = zoneX - PANEL_WIDTH;
            int btnIdx = g_presetPanel.HitTest(relX, my);
            for (int i = 0; i < g_presetPanel.GetButtonCount(); i++) {
                if (i == btnIdx) {
                    if (!g_presetPanel.GetButton(i).hover) {
                        g_presetPanel.SetButtonHover(i, true);
                        RECT r2; GetClientRect(hWnd, &r2);
                        r2.left = glw + PANEL_WIDTH;
                        InvalidateRect(hWnd, &r2, FALSE);
                    }
                } else if (g_presetPanel.GetButton(i).hover) {
                    g_presetPanel.SetButtonHover(i, false);
                    RECT r2; GetClientRect(hWnd, &r2);
                    r2.left = glw + PANEL_WIDTH;
                    InvalidateRect(hWnd, &r2, FALSE);
                }
            }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int glw = g_glWidth;

        if (mx < glw) break;

        int zoneX = mx - glw;
        int actionId = 0, sub = 0;
        int timerId = 0;

        if (zoneX < PANEL_WIDTH) {
            // 属性面板
            int btnIdx = g_panel.HitTest(zoneX, my);
            if (btnIdx >= 0) {
                g_panel.SetButtonPressed(btnIdx, true);
                if (g_panel.GetButton(btnIdx).type == BTN_DIRECTION)
                    sub = g_panel.HitTestDirSub(btnIdx, zoneX, my);
                if (g_panel.GetButton(btnIdx).type == BTN_TOGGLE)
                    sub = 1;
                actionId = g_panel.GetButton(btnIdx).actionId;
                timerId = 100 + btnIdx;
            }
            RECT r3; GetClientRect(hWnd, &r3);
            r3.left = glw; r3.right = glw + PANEL_WIDTH;
            InvalidateRect(hWnd, &r3, FALSE);
        } else {
            // 预置模型面板
            int relX = zoneX - PANEL_WIDTH;
            int btnIdx = g_presetPanel.HitTest(relX, my);
            if (btnIdx >= 0) {
                g_presetPanel.SetButtonPressed(btnIdx, true);
                actionId = g_presetPanel.GetButton(btnIdx).actionId;
                timerId = 200 + btnIdx;
            }
            RECT r3; GetClientRect(hWnd, &r3);
            r3.left = glw + PANEL_WIDTH;
            InvalidateRect(hWnd, &r3, FALSE);
        }

        if (actionId != 0) {
            ExecuteAction(actionId, sub);
            InvalidateRect(g_hGLWnd, NULL, FALSE);
            SetTimer(hWnd, timerId, 120, NULL);
        }
        break;
    }

    case WM_TIMER: {
        int rawId = (int)wParam;
        // 属性面板: ID = 100 + btnIdx
        int propId = rawId - 100;
        if (propId >= 0 && propId < g_panel.GetButtonCount()) {
            g_panel.SetButtonPressed(propId, false);
            RECT r4; GetClientRect(hWnd, &r4);
            r4.left = g_glWidth; r4.right = g_glWidth + PANEL_WIDTH;
            InvalidateRect(hWnd, &r4, FALSE);
            KillTimer(hWnd, wParam);
        }
        // 预置面板: ID = 200 + btnIdx
        int presetId = rawId - 200;
        if (presetId >= 0 && presetId < g_presetPanel.GetButtonCount()) {
            g_presetPanel.SetButtonPressed(presetId, false);
            RECT r4; GetClientRect(hWnd, &r4);
            r4.left = g_glWidth + PANEL_WIDTH;
            InvalidateRect(hWnd, &r4, FALSE);
            KillTimer(hWnd, wParam);
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        int zoneX = pt.x - g_glWidth;
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        if (zoneX >= 0 && zoneX < PANEL_WIDTH) {
            // 属性面板滚动
            g_panel.Scroll(delta);
            RECT r5; GetClientRect(hWnd, &r5);
            r5.left = g_glWidth; r5.right = g_glWidth + PANEL_WIDTH;
            InvalidateRect(hWnd, &r5, FALSE);
        } else if (zoneX >= PANEL_WIDTH) {
            // 预置面板滚动
            g_presetPanel.Scroll(delta);
            RECT r5; GetClientRect(hWnd, &r5);
            r5.left = g_glWidth + PANEL_WIDTH;
            InvalidateRect(hWnd, &r5, FALSE);
        }
        break;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_SPACE:  ExecuteAction(AID_TOGGLE_WIREFRAME, 1); break;
        case 'V':       if (g_isOrtho)  ExecuteAction(AID_TOGGLE_PROJECTION, 1); break;
        case 'v':       if (!g_isOrtho) ExecuteAction(AID_TOGGLE_PROJECTION, 1); break;
        case 'O': case 'o': ExecuteAction(AID_OPEN_MODEL, 0); break;
        case 'S': case 's': ExecuteAction(AID_RESET_ALL, 0); break;
        case 'Q': case 'q': ExecuteAction(AID_QUIT, 0); break;

        // 细分级别快捷键
        case '0': SetSubdivisionLevel(0); break;
        case '1': SetSubdivisionLevel(1); break;
        case '2': SetSubdivisionLevel(2); break;
        case '3': SetSubdivisionLevel(3); break;
        case '4': SetSubdivisionLevel(4); break;

        // 级别增减
        case VK_OEM_4: // '[' 键（无 Shift）
            if (g_subdivisionLevel > 0) SetSubdivisionLevel(g_subdivisionLevel - 1);
            break;
        case VK_OEM_6: // ']' 键（无 Shift）
            if (g_subdivisionLevel < 4) SetSubdivisionLevel(g_subdivisionLevel + 1);
            break;

        // 切换算法
        case 'M': case 'm':
            ExecuteAction(AID_SUBDIV_ALGORITHM, 1);
            break;
        }

        // 刷新两个面板
        RECT r;
        GetClientRect(hWnd, &r);
        r.left = g_glWidth;
        InvalidateRect(hWnd, &r, FALSE);
        break;
    }

    case WM_DESTROY:
        GdiplusShutdown(g_gdiToken);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
// WinMain - 入口点
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // 解析命令行
    char filename[MAX_PATH] = "";

    // 注册主窗口类
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ModelViewerMain";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"窗口注册失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 创建主窗口
    int winW = 1200;
    int winH = 750;
    HWND hWnd = CreateWindowEx(
        0,
        L"ModelViewerMain", L"Subdivision Surface Viewer - Loop & Catmull-Clark",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBox(NULL, L"窗口创建失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
