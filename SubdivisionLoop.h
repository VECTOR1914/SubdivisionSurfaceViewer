// ============================================================
// SubdivisionLoop.h - Loop 细分算法
// 适用于三角形网格的逼近型细分
//
// 算法原理:
//   边点 (新顶点):  内部边 = 3/8*(v0+v1) + 1/8*(v2+v3)
//                   边界边 = 1/2*(v0+v1)
//   顶点更新:       v' = (1 - n*β)*v + β*Σ(neighbors)
//                   其中 β = 1/n * (5/8 - (3/8 + 1/4*cos(2π/n))²)
// ============================================================
#ifndef SubdivisionLoop_H
#define SubdivisionLoop_H

#include "SubdivisionBase.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>

// ============================================================
// Loop 细分内部数据结构
// ============================================================
namespace loop {

struct Vertex {
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);

    std::vector<unsigned int> n_face;       // 邻接面索引
    std::vector<unsigned int> n_vertex;     // 邻接顶点索引（非边界）
    std::vector<unsigned int> b_vertex;     // 边界邻接顶点索引

    bool is_border = false;
};

struct Face {
    unsigned int v[3] = { 0, 0, 0 };        // 3 个顶点索引
    std::vector<unsigned int> n_edge;        // 3 条边索引
};

struct Edge {
    unsigned int v1 = 0;                     // 端点 1 索引
    unsigned int v2 = 0;                     // 端点 2 索引
    std::vector<unsigned int> n_face;        // 邻接面索引（1=边界, 2=内部）
    bool is_border = true;
    unsigned int mid = 0;                    // 细分后的新顶点索引（临时）
};

} // namespace loop

// ============================================================
// LoopSubdivision 类
// ============================================================
class LoopSubdivision : public SubdivisionBase
{
public:
    // ---- SubdivisionBase 接口 ----
    bool LoadFromMesh(const Mesh& mesh) override;
    void SubdivideOnce() override;
    Mesh ToMesh() override;

    unsigned int GetVertexCount() const override { return (unsigned int)m_vertices.size(); }
    unsigned int GetFaceCount() const override { return (unsigned int)m_faces.size(); }

private:
    std::vector<loop::Vertex> m_vertices;
    std::vector<loop::Edge>   m_edges;
    std::vector<loop::Face>   m_faces;

    // 辅助：构建邻接关系
    void BuildAdjacency();
};

#endif // SubdivisionLoop_H
