// ============================================================
// Mesh.h - 网格数据结构与 OBJ 加载
// 存储顶点位置（flat array）+ 结构化索引面
// 替代原来的 Model::vertex_list + Model::s_list (string faces)
// ============================================================
#ifndef Mesh_H
#define Mesh_H

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>

// ============================================================
// IndexedFace - 一个面的顶点索引列表（0-based）
// ============================================================
struct IndexedFace {
    std::vector<unsigned int> vertex_indices;
};

// ============================================================
// Mesh - 网格数据
// positions: 扁平化的 x,y,z 坐标数组（兼容旧 OpenGL 渲染代码）
// faces:     结构化的索引面数组
// ============================================================
class Mesh
{
public:
    // ---- 顶点数据（flat array，兼容旧代码） ----
    std::vector<float> positions;          // x0,y0,z0, x1,y1,z1, ...
    std::vector<IndexedFace> faces;        // 结构化索引面

    // ---- 包围盒 ----
    float min_x, max_x;
    float min_y, max_y;
    float min_z, max_z;
    float center_x, center_y, center_z;
    float max_scale;

    // ---- 构造函数 ----
    Mesh();
    void Clear();

    // ---- OBJ 加载 ----
    bool LoadFromOBJ(const std::string& filepath);
    bool LoadFromMemory(const char* data);       // 从内存字符串加载 OBJ
    bool LoadFromResource(HINSTANCE hInst, int resId); // 从 Windows 资源加载 OBJ

    // ---- 归一化（居中 + 缩放）----
    void Normalize();

    // ---- 扇形三角化（Loop 细分需要）----
    void Triangulate();

    // ---- 查询 ----
    unsigned int VertexCount() const;
    unsigned int FaceCount() const;
    void GetVertex(unsigned int idx, float& x, float& y, float& z) const;

    // ---- 保存 ----
    bool SaveToOBJ(const std::string& filepath) const;

private:
    void CalculateBoundingBox();
    void CalculateCenter();
};

// ============================================================
// 内联实现
// ============================================================
inline Mesh::Mesh()
{
    Clear();
}

inline void Mesh::Clear()
{
    positions.clear();
    faces.clear();
    min_x = min_y = min_z = 0.0f;
    max_x = max_y = max_z = 0.0f;
    center_x = center_y = center_z = 0.0f;
    max_scale = 1.0f;
}

inline unsigned int Mesh::VertexCount() const
{
    return (unsigned int)(positions.size() / 3);
}

inline unsigned int Mesh::FaceCount() const
{
    return (unsigned int)faces.size();
}

inline void Mesh::GetVertex(unsigned int idx, float& x, float& y, float& z) const
{
    unsigned int i = idx * 3;
    if (i + 2 < positions.size()) {
        x = positions[i];
        y = positions[i + 1];
        z = positions[i + 2];
    }
}

// ============================================================
// 计算包围盒
// ============================================================
inline void Mesh::CalculateBoundingBox()
{
    if (positions.empty()) return;

    max_x = min_x = positions[0];
    max_y = min_y = positions[1];
    max_z = min_z = positions[2];

    for (unsigned int i = 0; i < positions.size(); i += 3) {
        float x = positions[i];
        float y = positions[i + 1];
        float z = positions[i + 2];

        if (x > max_x) max_x = x;
        if (x < min_x) min_x = x;
        if (y > max_y) max_y = y;
        if (y < min_y) min_y = y;
        if (z > max_z) max_z = z;
        if (z < min_z) min_z = z;
    }

    float sx = max_x - min_x;
    float sy = max_y - min_y;
    float sz = max_z - min_z;
    max_scale = sx;
    if (sy > max_scale) max_scale = sy;
    if (sz > max_scale) max_scale = sz;
    if (max_scale < 0.0001f) max_scale = 1.0f;
}

// ============================================================
// 计算中心点
// ============================================================
inline void Mesh::CalculateCenter()
{
    if (positions.empty()) {
        center_x = center_y = center_z = 0.0f;
        return;
    }
    center_x = center_y = center_z = 0.0f;
    unsigned int n = VertexCount();
    for (unsigned int i = 0; i < positions.size(); i += 3) {
        center_x += positions[i];
        center_y += positions[i + 1];
        center_z += positions[i + 2];
    }
    center_x /= (float)n;
    center_y /= (float)n;
    center_z /= (float)n;
}

// ============================================================
// 归一化：居中 + 缩放到单位大小 + 平移至 Z=-2
// 与原 Model::applyTransfToMatrix() 行为一致
// ============================================================
inline void Mesh::Normalize()
{
    if (positions.empty()) return;

    CalculateBoundingBox();
    CalculateCenter();

    // 平移居中
    for (unsigned int i = 0; i < positions.size(); i += 3) {
        positions[i]     -= center_x;
        positions[i + 1] -= center_y;
        positions[i + 2] -= center_z;
    }

    // 缩放
    for (unsigned int i = 0; i < positions.size(); i++) {
        positions[i] /= max_scale;
    }

    // 平移至 Z = -2
    for (unsigned int i = 0; i < positions.size(); i += 3) {
        positions[i + 2] -= 2.0f;
    }

    // 重新计算包围盒
    CalculateBoundingBox();
    CalculateCenter();
}

// ============================================================
// 扇形三角化：将 n>3 的面转换为三角形
// 对于面 v0, v1, v2, ..., vn-1，生成 (v0,v1,v2), (v0,v2,v3), ...
// ============================================================
inline void Mesh::Triangulate()
{
    std::vector<IndexedFace> newFaces;
    newFaces.reserve(faces.size() * 2);

    for (size_t i = 0; i < faces.size(); i++) {
        const IndexedFace& face = faces[i];
        if (face.vertex_indices.size() == 3) {
            // 已经是三角形，直接保留
            newFaces.push_back(face);
        }
        else if (face.vertex_indices.size() >= 4) {
            // 扇形三角化
            for (size_t j = 1; j + 1 < face.vertex_indices.size(); j++) {
                IndexedFace tri;
                tri.vertex_indices.push_back(face.vertex_indices[0]);
                tri.vertex_indices.push_back(face.vertex_indices[j]);
                tri.vertex_indices.push_back(face.vertex_indices[j + 1]);
                newFaces.push_back(tri);
            }
        }
        // 忽略退化面（< 3 顶点）
    }

    faces.swap(newFaces);
}

#endif // Mesh_H
