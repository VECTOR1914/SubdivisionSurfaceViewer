// ============================================================
// Renderer.h - OpenGL 渲染器
// 封装 OpenGL 初始化、窗口 resize、display list 构建、场景渲染
// 从 main.cpp 提取
// ============================================================
#ifndef Renderer_H
#define Renderer_H

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "Mesh.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // ---- 生命周期 ----
    bool Initialize(HWND hWnd);       // 创建 GL 上下文
    void Resize(int w, int h);        // 记录窗口大小
    void Shutdown();                  // 释放 GL 资源

    // ---- 渲染（调用者需确保已 wglMakeCurrent）----
    void Render(const Mesh& mesh,
                float cameraDistance, float cameraRotX, float cameraRotY,
                float cameraX, float cameraY,
                float modelX, float modelY, float modelZ,
                float modelRotX, float modelRotY, float modelRotZ,
                bool wireframe, bool orthoProjection,
                int subdivisionLevel = 0);

    // ---- Display List ----
    void BuildDisplayList(const Mesh& mesh);  // 从 Mesh 重建 display list

    // ---- GL 上下文访问 ----
    HGLRC GetGLRC() const { return m_hGLRC; }

private:
    HWND   m_hWnd;          // 关联的窗口句柄
    HGLRC  m_hGLRC;         // GL 渲染上下文（持久）
    GLuint m_displayList;   // 模型 display list
    bool   m_initialized;
    int    m_width;
    int    m_height;

    void SetupLighting();
    void SetupMaterial();
};

#endif // Renderer_H
