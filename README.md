# 🎨 Subdivision Surface Viewer — 细分曲面交互式查看器

> **一个 EXE，三种经典算法，十六个精美模型 — 从粗糙多边形到光滑曲面的魔法**

<p align="center">
  <img src="https://img.shields.io/badge/Language-C++-00599C?style=flat-square&logo=c%2B%2B" alt="C++">
  <img src="https://img.shields.io/badge/Graphics-OpenGL-5587A2?style=flat-square&logo=opengl" alt="OpenGL">
  <img src="https://img.shields.io/badge/UI-GDI+-5C2D91?style=flat-square" alt="GDI+">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/platform-Windows-blue?style=flat-square&logo=windows" alt="Windows">
</p>

---

## ✨ 项目简介

**Subdivision Surface Viewer** 是一个 Windows 桌面端 3D 细分曲面交互式查看器。它将计算机图形学中最优雅的**细分曲面算法**（Subdivision Surface）以「实时可交互」的方式呈现——你只需拖拽鼠标，就能从任意角度观察网格如何从棱角分明的低面数模型，逐步演化为光滑细腻的高面数曲面。

本项目**内置 16 个预置 3D 模型**（Stanford Bunny、恐龙、茶杯、人脸等），**所有模型数据嵌入在 EXE 资源段中**，单个可执行文件即可运行，无需安装、无需联网、无需额外数据文件。

---

## 🧠 三种细分算法，一网打尽

| 算法 | 类型 | 适用网格 | 特点 |
|------|------|----------|------|
| **Loop** | 逼近型 | 三角形 | Charles Loop 1987 年提出，C² 连续（规则区域），光滑度极高 |
| **Catmull-Clark** | 逼近型 | 四边形 / 多边形 | Pixar 工业标准，所有电影级曲面建模的基石 |
| **Butterfly** 🆕 | 插值型 | 三角形 | Dyn, Levin & Gregory 1990 年提出，原始顶点精确保持，8 点蝶形模板 |

> 💡 **三种算法横向对比**：加载同一个模型，按 `M` 键一键切换算法，直观感受逼近型与插值型的本质区别。

---

## 🎬 核心特性

- 🖱️ **轨道相机** — 左键旋转、右键平移、滚轮缩放，流畅 60 FPS
- 🔢 **0–4 级细分** — 按 `[` / `]` 逐级切换，每级面数 4× 增长，肉眼可见的光滑进化
- 📦 **16 个内置模型** — 单文件 EXE，模型嵌入资源段，即开即用
- 🎨 **深色主题 UI** — GDI+ 手绘深色面板，科技感十足
- 📂 **OBJ 导入/导出** — 支持加载外部 Wavefront OBJ，细分后导出保存
- 🧵 **线框 / 填充切换** — 按空格键在网格线框与实体填充之间切换
- 📐 **正交 / 透视投影** — 按 `V` 键切换，工程制图 or 真实透视随你选
- 📊 **实时信息显示** — FPS、当前算法、细分级别、顶点数、面数一目了然

---

## ⌨️ 操作速查

### 🖱️ 鼠标

| 操作 | 功能 |
|------|------|
| 左键拖拽 | 旋转视角 |
| 右键拖拽 | 平移视角 |
| 滚轮 | 缩放 |
| 右侧面板滚轮 | 滚动控制面板 |

### ⌨️ 键盘

| 按键 | 功能 |
|------|------|
| `0` – `4` | 直接设置细分级别 |
| `[` / `]` | 降低 / 提高细分级别 |
| `M` | 切换细分算法（Loop → Catmull-Clark → Butterfly） |
| `Space` | 线框 ⇄ 填充 |
| `V` | 正交 ⇄ 透视 |
| `O` | 打开外部 OBJ 文件 |
| `S` | 重置视角 |
| `Q` | 退出 |

---

## 🔧 编译 & 运行

### 环境要求

- **Visual Studio 2019+**（MSVC 工具链）
- **Windows SDK**（含 OpenGL、GDI+ 头文件）
- 无需额外依赖 — `glm` 数学库已包含在项目中（header-only）

### 三步运行

```bash
# 1. 打开解决方案
双击 ModelViewer.sln

# 2. 选择配置
Debug / Release, x64 / Win32

# 3. 编译运行
按 F5
```

### 链接库

| 库 | 用途 |
|----|------|
| `opengl32.lib` | OpenGL 渲染 |
| `glu32.lib` | GLU 辅助（gluPerspective 等） |
| `gdiplus.lib` | 右侧交互面板 |
| `comctl32.lib` | Windows 通用控件 |
| `comdlg32.lib` | 文件打开/保存对话框 |

---

## 📁 项目结构

```
.
├── main.cpp                      # 🏠 Win32 主程序入口（窗口 + 消息循环）
├── Mesh.h / Mesh.cpp             # 📐 网格数据结构 + OBJ 文件加载/保存
├── Renderer.h / Renderer.cpp     # 🎨 OpenGL 渲染器（光照、颜色、线框）
├── Camera.h / Camera.cpp         # 📷 轨道相机（Arcball 旋转 + 平移 + 缩放）
├── Panel.h / Panel.cpp           # 🎛️  GDI+ 右侧控制面板（按钮、滑块）
├── PresetPanel.h / PresetPanel.cpp # 📋 预置模型选择面板
├── SubdivisionBase.h             # 🧬 细分算法抽象基类
├── SubdivisionLoop.h / .cpp      # 🔺 Loop 细分（逼近型 · 三角形）
├── SubdivisionCatmullClark.h/.cpp # 🔲 Catmull-Clark 细分（逼近型 · 多边形）
├── SubdivisionButterfly.h / .cpp  # 🦋 Butterfly 细分（插值型 · 三角形）
├── glm/                          # 📦 glm 数学库（header-only）
├── models/                       # 🗿 示例 OBJ 模型（16 个）
├── scripts/
│   ├── embed_models.py           # 🔨 模型嵌入脚本（OBJ → 资源文件）
│   └── md2docx.py                # 📝 Markdown → Word 转换脚本
└── ModelViewer.sln               # 🏗️  Visual Studio 解决方案
```

---

## 🌟 项目亮点

### 1️⃣ 教科书级别的细分算法实现

代码结构清晰，每种算法独立成类，继承自 `SubdivisionBase` 抽象基类。无论是学习图形学课程，还是理解 Pixar 动画背后的数学原理，这都是极佳的参考实现。

### 2️⃣ 单文件分发，零门槛体验

所有 16 个模型（总计约 27 MB）通过 Windows 资源机制嵌入到 EXE 中。发给朋友一个 EXE，他就能立刻体验细分曲面的魅力——不需要 "先下载模型文件放到 models/ 目录" 之类的前提条件。

### 3️⃣ 深色科技风 UI

抛弃 MFC/Qt 等重型框架，直接用 GDI+ 手绘了一份深色主题交互界面：圆角按钮、hover 高亮、绿色选中态——简约而不简陋。

### 4️⃣ 内置模型丰富

Stanford Bunny（计算机图形学「Hello World」）、Utah Teapot（渲染测试经典）、恐龙、猫、狗、鹿、人脸、奶牛、太空飞船……从动物到器物，覆盖多种几何拓扑。

---

## 📸 效果预览

> 加载任意内置模型，按 `[` 逐级细分，观察低面数网格 → 光滑曲面的过程。
>
> *（截图请自行补充到仓库中 — 建议放 3–4 张：原始网格 vs 4 级细分、三种算法对比、深色面板 UI、线框模式）*

---

## 🧪 技术细节

### Loop 细分

- 每条边生成新顶点：`3/8(v₀+v₁) + 1/8(v₂+v₃)`
- 原顶点更新：`(1 - nβ)v + β·Σ neighbors`，其中 `β = 1/n · (5/8 - (3/8 + 1/4·cos(2π/n))²)`
- 极限曲面 C² 连续（规则顶点处）

### Catmull-Clark 细分

- 面点 = 面顶点均值
- 边点 = (边端点 + 相邻面点均值) / 2
- 新顶点 = (F + 2R + (n-3)P) / n
- 每四边形 → 4 子面，每三角形 → 3 子面

### Butterfly 细分（插值型）

- 规则顶点（价 = 6）：8 点蝶形模板
- 奇异顶点：Modified Butterfly 权重（基于价和邻居索引的三角函数）
- 原始顶点坐标**完全保留**，仅新增边点

---

## 🙋 适用场景

- 🎓 **计算机图形学教学** — 直观展示细分曲面原理
- 🔬 **算法对比研究** — 同一模型三种算法实时切换
- 🛠️ **3D 模型快速预览** — 加载 OBJ 文件，查看细分效果
- 💼 **面试作品展示** — 体现你对图形学、OpenGL、C++ 的综合掌握

---

## 📄 License

MIT © 2025

---

## ⭐ 如果这个项目对你有帮助

如果这个项目帮你理解了细分曲面，或者在图形学课上救了急——**点个 Star ⭐** 就是对作者最大的鼓励！也欢迎 Fork 并提交 PR 来完善它。
