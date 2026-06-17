// ============================================================
// SubdivisionButterfly.h - Butterfly 细分算法 (插值型)
//
// Butterfly 是插值型细分，原始顶点保持不变，仅计算新边点。
// 适用于三角形网格。
//
// 边点计算:
//   规则顶点(valence=6): 8 点蝶形模板
//     edge = 1/2(v0+v1) + 1/8(v2+v3) - 1/16(v4+v5+v6+v7)
//   奇异顶点(valence≠6): Modified Butterfly 权重
//     s_j = (1/k)(1/4 + cos(2πj/k) + 1/2·cos(4πj/k))
//   边界边: 简单中点
// ============================================================
#ifndef SubdivisionButterfly_H
#define SubdivisionButterfly_H

#include "SubdivisionBase.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>

// ============================================================
// Butterfly 内部数据结构 (同 Loop，三角形网格)
// ============================================================
namespace butterfly {

struct Vertex {
    glm::vec3 pos;
    std::vector<unsigned int> n_face;       // 邻接面
    std::vector<unsigned int> n_vertex;     // 邻接顶点 (非边界)
    std::vector<unsigned int> b_vertex;     // 边界邻接顶点
    bool is_border = false;
};

struct Face {
    unsigned int v[3] = { 0, 0, 0 };
    std::vector<unsigned int> n_edge;
};

struct Edge {
    unsigned int v1 = 0;
    unsigned int v2 = 0;
    std::vector<unsigned int> n_face;       // 1=边界, 2=内部
    bool is_border = true;
    unsigned int mid = 0;                   // 细分后新顶点索引 (临时)
};

} // namespace butterfly

// ============================================================
// SubdivisionButterfly 类
// ============================================================
class SubdivisionButterfly : public SubdivisionBase
{
public:
    bool LoadFromMesh(const Mesh& mesh) override;
    void SubdivideOnce() override;
    Mesh ToMesh() override;

    unsigned int GetVertexCount() const override { return (unsigned int)m_vertices.size(); }
    unsigned int GetFaceCount() const override { return (unsigned int)m_faces.size(); }

private:
    std::vector<butterfly::Vertex> m_vertices;
    std::vector<butterfly::Edge>   m_edges;
    std::vector<butterfly::Face>   m_faces;

    void BuildAdjacency();

    // Modified Butterfly 权重: 对于 valence k 的奇异顶点，返回邻居 j 的权重
    float ModifiedButterflyWeight(int k, int j) const;
};

#endif // SubdivisionButterfly_H
