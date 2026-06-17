// ============================================================
// Renderer.cpp - OpenGL 渲染器实现
// ============================================================
#include "Renderer.h"
#include <cmath>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// ============================================================
// 构造 / 析构
// ============================================================
Renderer::Renderer()
    : m_hWnd(NULL)
    , m_hGLRC(NULL)
    , m_displayList(0)
    , m_initialized(false)
    , m_width(800)
    , m_height(600)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

// ============================================================
// Initialize - 设置像素格式、创建 GL 上下文、光照
// 调用者需要在调用后 ReleaseDC
// ============================================================
bool Renderer::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,     // color depth
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        32,     // depth buffer
        0, 0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };

    HDC hdc = GetDC(m_hWnd);
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);
    m_hGLRC = wglCreateContext(hdc);
    wglMakeCurrent(hdc, m_hGLRC);

    // ---- 基础 GL 状态 ----
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    SetupLighting();
    SetupMaterial();

    glShadeModel(GL_SMOOTH);

    wglMakeCurrent(NULL, NULL);
    ReleaseDC(m_hWnd, hdc);

    m_initialized = true;
    return true;
}

// ============================================================
// SetupLighting - 双光源设置
// ============================================================
void Renderer::SetupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // Key light (右上前方)
    GLfloat keyAmbient[]  = { 0.12f, 0.12f, 0.14f, 1.0f };
    GLfloat keyDiffuse[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat keySpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat keyPos[]      = { 5.0f, 8.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  keyAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  keyDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, keySpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, keyPos);

    // Fill light (左下后方，较暗，无镜面)
    GLfloat fillAmbient[]  = { 0.08f, 0.08f, 0.10f, 1.0f };
    GLfloat fillDiffuse[]  = { 0.30f, 0.30f, 0.35f, 1.0f };
    GLfloat fillSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat fillPos[]      = { -4.0f, -1.0f, -4.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT,  fillAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  fillDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpecular);
    glLightfv(GL_LIGHT1, GL_POSITION, fillPos);
}

// ============================================================
// SetupMaterial - 材质属性
// ============================================================
void Renderer::SetupMaterial()
{
    GLfloat matAmbient[]  = { 0.25f, 0.35f, 0.5f, 1.0f };
    GLfloat matDiffuse[]  = { 0.4f, 0.55f, 0.75f, 1.0f };
    GLfloat matSpecular[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpecular);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 60.0f);
}

// ============================================================
// Resize - 记录窗口大小
// ============================================================
void Renderer::Resize(int w, int h)
{
    if (h == 0) h = 1;
    m_width = w;
    m_height = h;
}

// ============================================================
// Shutdown - 释放 OpenGL 资源
// ============================================================
void Renderer::Shutdown()
{
    if (m_hGLRC) {
        // 在释放前删除 display list
        if (m_displayList != 0) {
            HDC hdc = GetDC(m_hWnd);
            if (hdc) {
                wglMakeCurrent(hdc, m_hGLRC);
                glDeleteLists(m_displayList, 1);
                m_displayList = 0;
                wglMakeCurrent(NULL, NULL);
                ReleaseDC(m_hWnd, hdc);
            }
        }
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
    }
    m_initialized = false;
}

// ============================================================
// BuildDisplayList - 从 Mesh 构建 OpenGL display list
// 每个面使用 GL_POLYGON，用 Newell 方法计算面法线
// 必须在有效的 GL 上下文中调用
// ============================================================
void Renderer::BuildDisplayList(const Mesh& mesh)
{
    if (!m_hGLRC) return;

    // 删除旧的 display list
    if (m_displayList != 0) {
        glDeleteLists(m_displayList, 1);
        m_displayList = 0;
    }

    if (mesh.faces.empty()) return;

    m_displayList = glGenLists(1);
    glNewList(m_displayList, GL_COMPILE);

    const std::vector<float>& vtx = mesh.positions;

    for (size_t h = 0; h < mesh.faces.size(); h++) {
        const IndexedFace& face = mesh.faces[h];
        const std::vector<unsigned int>& idx = face.vertex_indices;

        if (idx.size() < 3) continue;

        // ---- 用 Newell 方法计算面法线 ----
        float nx = 0.0f, ny = 0.0f, nz = 0.0f;
        for (size_t i = 0; i < idx.size(); i++) {
            size_t j = (i + 1) % idx.size();
            unsigned int a = idx[i] * 3;
            unsigned int b = idx[j] * 3;
            float ax = vtx[a], ay = vtx[a + 1], az = vtx[a + 2];
            float bx = vtx[b], by = vtx[b + 1], bz = vtx[b + 2];
            nx += (ay - by) * (az + bz);
            ny += (az - bz) * (ax + bx);
            nz += (ax - bx) * (ay + by);
        }
        float len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; }

        glBegin(GL_POLYGON);
        glNormal3f(nx, ny, nz);
        for (size_t i = 0; i < idx.size(); i++) {
            unsigned int vi = idx[i] * 3;
            glVertex3f(vtx[vi], vtx[vi + 1], vtx[vi + 2]);
        }
        glEnd();
    }

    glEndList();
}

// ============================================================
// Render - 主渲染函数
// 调用者负责：
//   1. 获取 HDC 并 wglMakeCurrent(hdc, m_hGLRC)
//   2. 调用此函数
//   3. SwapBuffers(hdc)
//   4. wglMakeCurrent(NULL, NULL) + ReleaseDC
// ============================================================
void Renderer::Render(const Mesh& mesh,
                       float cameraDistance, float cameraRotX, float cameraRotY,
                       float cameraX, float cameraY,
                       float modelX, float modelY, float modelZ,
                       float modelRotX, float modelRotY, float modelRotZ,
                       bool wireframe, bool orthoProjection,
                       int subdivisionLevel)
{
    if (!m_initialized) return;

    // ---- 根据细分级别设置材质颜色（区分原始/细分）----
    if (subdivisionLevel == 0) {
        // 原始模型：蓝色调
        GLfloat matAmbient[]  = { 0.25f, 0.35f, 0.5f, 1.0f };
        GLfloat matDiffuse[]  = { 0.4f, 0.55f, 0.75f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    } else {
        // 细分模型：金/绿色调
        GLfloat matAmbient[]  = { 0.3f, 0.25f, 0.15f, 1.0f };
        GLfloat matDiffuse[]  = { 0.7f, 0.6f, 0.3f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    }

    // ---- 视口（必须在每帧设置，否则会停留在初始小尺寸）----
    glViewport(0, 0, m_width, m_height);

    // ---- 投影矩阵 ----
    float aspect = (m_height > 0) ? (float)m_width / (float)m_height : 1.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (orthoProjection) {
        float s = cameraDistance * 0.35f;
        if (s < 0.01f) s = 0.01f;
        glOrtho(-s * aspect, s * aspect, -s, s, 0.01, 200.0);
    } else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ---- 多边形模式 ----
    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ---- 轨道相机 ----
    float ocx = modelX;
    float ocy = modelY;
    float ocz = -2.0f + modelZ;

    float rx = cameraRotX * 3.14159f / 180.0f;
    float ry = cameraRotY * 3.14159f / 180.0f;
    float d  = cameraDistance;

    float eyeX = ocx + d * sinf(ry) * cosf(rx);
    float eyeY = ocy + d * sinf(rx);
    float eyeZ = ocz + d * cosf(ry) * cosf(rx);

    gluLookAt(eyeX, eyeY, eyeZ,   ocx, ocy, ocz,   0.0f, 1.0f, 0.0f);

    // ---- 光源位置（眼睛空间）----
    GLfloat lightPos[] = { 5.0f, 8.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ---- 模型变换（绕自身中心旋转）----
    glTranslatef(ocx, ocy, ocz);
    glRotatef(modelRotX, 1, 0, 0);
    glRotatef(modelRotY, 0, 1, 0);
    glRotatef(modelRotZ, 0, 0, 1);
    glTranslatef(-ocx, -ocy, -ocz);

    // ---- 绘制模型 ----
    if (m_displayList != 0) {
        glCallList(m_displayList);
    }
}
