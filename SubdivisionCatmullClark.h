// ============================================================
// SubdivisionCatmullClark.h - Catmull-Clark 细分算法
// 适用于四边形/任意多边形网格的逼近型细分
//
// 算法原理:
//   面点 (Face point):    面的所有顶点平均值
//   边点 (Edge point):    边中点与相邻面点平均值的中点
//   顶点更新 (Vertex):    (n-3)/n * v + 1/n * avg_face_points + 2/n * avg_edge_midpoints
//   边界边:                边点为边中点
//   边界顶点:              (v + 0.5*(v+n0) + 0.5*(v+n1)) / 3
// ============================================================
#ifndef SubdivisionCatmullClark_H
#define SubdivisionCatmullClark_H

#include "SubdivisionBase.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>

// ============================================================
// Catmull-Clark 细分内部数据结构
// ============================================================
namespace catmull {

struct Vertex {
    glm::vec3 pos;
    std::vector<unsigned int> n_face;       // 邻接面索引
    std::vector<unsigned int> n_vertex;     // 邻接顶点索引（非边界）
    std::vector<unsigned int> b_vertex;     // 边界邻接顶点索引
    bool is_border = false;
};

struct Face {
    unsigned int v[4] = { 0, 0, 0, 0 };     // 4 个顶点索引
    bool is_triangle = false;                // 三角形时 v[3] 不使用
    std::vector<unsigned int> n_edge;        // 邻接边索引
    int fp_idx = -1;                         // 新面点索引（临时）
};

struct Edge {
    unsigned int v1 = 0;
    unsigned int v2 = 0;
    std::vector<unsigned int> n_face;        // 邻接面索引
    bool is_border = true;
    int ep_idx = -1;                         // 新边点索引（临时）
};

} // namespace catmull

// ============================================================
// CatmullClarkSubdivision 类
// ============================================================
class CatmullClarkSubdivision : public SubdivisionBase
{
public:
    // ---- SubdivisionBase 接口 ----
    bool LoadFromMesh(const Mesh& mesh) override;
    void SubdivideOnce() override;
    Mesh ToMesh() override;

    unsigned int GetVertexCount() const override { return (unsigned int)m_vertices.size(); }
    unsigned int GetFaceCount() const override { return (unsigned int)m_faces.size(); }

private:
    std::vector<catmull::Vertex> m_vertices;
    std::vector<catmull::Edge>   m_edges;
    std::vector<catmull::Face>   m_faces;

    void BuildAdjacency();

    // 辅助：获取边的有序索引对
    static std::pair<int, int> GetEdgeKey(int v1, int v2)
    {
        return (v1 < v2) ? std::pair<int,int>(v1, v2) : std::pair<int,int>(v2, v1);
    }
};

#endif // SubdivisionCatmullClark_H
