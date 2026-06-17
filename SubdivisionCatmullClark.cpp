// ============================================================
// SubdivisionCatmullClark.cpp - Catmull-Clark 细分算法实现
// 改编自 Sample project/Mesh-Subdivisions/src/subdivisions/catmull.cpp
// ============================================================
#include "SubdivisionCatmullClark.h"
#include <algorithm>
#include <iostream>
#include <cassert>

using namespace catmull;

// ============================================================
// LoadFromMesh - 从 Mesh 构建邻接数据结构
// 支持三角形和四边形面
// ============================================================
bool CatmullClarkSubdivision::LoadFromMesh(const Mesh& mesh)
{
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();

    if (mesh.faces.empty() || mesh.positions.empty())
        return false;

    std::map<std::pair<int, int>, int> edgeIndex;
    m_vertices.resize(mesh.VertexCount());

    // ---- 遍历所有面 ----
    for (unsigned int i = 0; i < mesh.faces.size(); i++) {
        const IndexedFace& curFace = mesh.faces[i];
        unsigned int faceIdx = (unsigned int)m_faces.size();
        Face f;

        unsigned int nv = (unsigned int)curFace.vertex_indices.size();
        if (nv < 3 || nv > 4) {
            // Catmull-Clark 仅直接支持三角形和四边形
            // n>4 的面应在调用前三角化
            continue;
        }

        f.is_triangle = (nv == 3);

        // ---- 构建顶点 ----
        for (unsigned int j = 0; j < nv; j++) {
            unsigned int vi = curFace.vertex_indices[j];
            f.v[j] = vi;

            float x, y, z;
            mesh.GetVertex(vi, x, y, z);
            m_vertices[vi].pos = glm::vec3(x, y, z);
            m_vertices[vi].n_face.push_back(faceIdx);
            m_vertices[vi].is_border = false;
        }
        if (nv == 3) f.v[3] = f.v[2]; // 三角形: v[3] = v[2]

        // ---- 构建边 ----
        for (unsigned int j = 0; j < nv; j++) {
            unsigned int vA = f.v[j];
            unsigned int vB = (j + 1 < nv) ? f.v[j + 1] : f.v[0];
            auto key = GetEdgeKey((int)vA, (int)vB);

            unsigned int edgeIdx;
            if (edgeIndex.find(key) == edgeIndex.end()) {
                // 新边
                edgeIdx = (unsigned int)m_edges.size();
                Edge e;
                e.v1 = key.first;
                e.v2 = key.second;
                m_edges.push_back(std::move(e));
                edgeIndex[key] = (int)edgeIdx;
                m_edges[edgeIdx].is_border = true;
            } else {
                edgeIdx = edgeIndex[key];
                m_edges[edgeIdx].is_border = false;
            }
            m_edges[edgeIdx].n_face.push_back(faceIdx);
            f.n_edge.push_back(edgeIdx);
        }

        m_faces.push_back(f);
    }

    // ---- 构建顶点邻接 ----
    BuildAdjacency();
    return true;
}

// ============================================================
// BuildAdjacency - 从边数据构建顶点邻接关系
// ============================================================
void CatmullClarkSubdivision::BuildAdjacency()
{
    for (size_t i = 0; i < m_edges.size(); i++) {
        const Edge& e = m_edges[i];
        if (e.is_border) {
            m_vertices[e.v1].is_border = true;
            m_vertices[e.v2].is_border = true;
            m_vertices[e.v1].b_vertex.push_back(e.v2);
            m_vertices[e.v2].b_vertex.push_back(e.v1);
        } else {
            m_vertices[e.v1].n_vertex.push_back(e.v2);
            m_vertices[e.v2].n_vertex.push_back(e.v1);
        }
    }
}

// ============================================================
// SubdivideOnce - 执行一次 Catmull-Clark 细分
// ============================================================
void CatmullClarkSubdivision::SubdivideOnce()
{
    std::vector<Vertex> oldVertices(m_vertices);
    std::vector<Edge>   oldEdges(m_edges);
    std::vector<Face>   oldFaces(m_faces);

    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
    m_vertices.resize(oldVertices.size());

    // ================================================================
    // 第一步: 计算面点 (face points)
    // 面点 = 该面所有顶点的平均值
    // ================================================================
    for (size_t i = 0; i < oldFaces.size(); i++) {
        Face& curFace = oldFaces[i];
        Vertex facePoint;
        unsigned int fp_idx = (unsigned int)m_vertices.size();
        curFace.fp_idx = (int)fp_idx;

        int n = curFace.is_triangle ? 3 : 4;
        glm::vec3 sum(0.0f);
        for (int j = 0; j < n; j++) {
            sum += oldVertices[curFace.v[j]].pos;
        }
        facePoint.pos = sum / (float)n;
        m_vertices.push_back(std::move(facePoint));
    }

    // ================================================================
    // 第二步: 计算边点 (edge points)
    // 内部边: (边中点 + 相邻面点平均值) / 2
    // 边界边: 边中点
    // ================================================================
    for (size_t i = 0; i < oldEdges.size(); i++) {
        Edge& curEdge = oldEdges[i];
        Vertex edgePoint;
        unsigned int ep_idx = (unsigned int)m_vertices.size();
        curEdge.ep_idx = (int)ep_idx;

        if (curEdge.is_border) {
            // 边界边: 边中点
            edgePoint.pos = 0.5f * oldVertices[curEdge.v1].pos
                          + 0.5f * oldVertices[curEdge.v2].pos;
            edgePoint.is_border = true;

            // 邻接关系
            edgePoint.b_vertex.push_back(curEdge.v1);
            edgePoint.b_vertex.push_back(curEdge.v2);
            edgePoint.n_vertex.push_back(curEdge.v1);
            edgePoint.n_vertex.push_back(curEdge.v2);
            unsigned int fp0 = oldFaces[curEdge.n_face[0]].fp_idx;
            edgePoint.n_vertex.push_back(fp0);

            m_vertices[curEdge.v1].b_vertex.push_back(ep_idx);
            m_vertices[curEdge.v2].b_vertex.push_back(ep_idx);
            m_vertices[curEdge.v1].n_vertex.push_back(ep_idx);
            m_vertices[curEdge.v2].n_vertex.push_back(ep_idx);
            m_vertices[fp0].b_vertex.push_back(ep_idx);
        } else {
            // 内部边: (0.5*(v1+v2) + 0.5*(fp0+fp1)) / 2
            glm::vec3 edgeMid = 0.5f * oldVertices[curEdge.v1].pos
                              + 0.5f * oldVertices[curEdge.v2].pos;
            unsigned int fp0_idx = oldFaces[curEdge.n_face[0]].fp_idx;
            unsigned int fp1_idx = oldFaces[curEdge.n_face[1]].fp_idx;
            glm::vec3 fp0 = m_vertices[fp0_idx].pos;
            glm::vec3 fp1 = m_vertices[fp1_idx].pos;

            edgePoint.pos = 0.5f * edgeMid + 0.25f * fp0 + 0.25f * fp1;
            edgePoint.is_border = false;

            edgePoint.n_vertex.push_back(curEdge.v1);
            edgePoint.n_vertex.push_back(curEdge.v2);
            edgePoint.n_vertex.push_back(fp0_idx);
            edgePoint.n_vertex.push_back(fp1_idx);

            m_vertices[curEdge.v1].n_vertex.push_back(ep_idx);
            m_vertices[curEdge.v2].n_vertex.push_back(ep_idx);
            m_vertices[fp0_idx].n_vertex.push_back(ep_idx);
            m_vertices[fp1_idx].n_vertex.push_back(ep_idx);
        }
        m_vertices.push_back(std::move(edgePoint));
    }

    // ================================================================
    // 第三步: 更新原始顶点位置
    // 内部顶点: (n-3)/n * v + 1/n * avg(face_points) + 2/n * avg(edge_midpoints)
    // 边界顶点: (v + 0.5*(v+b0) + 0.5*(v+b1)) / 3
    // ================================================================
    for (size_t i = 0; i < oldVertices.size(); i++) {
        const Vertex& oldV = oldVertices[i];
        Vertex& newV = m_vertices[i];

        if (oldV.is_border) {
            newV.is_border = true;
            assert(oldV.b_vertex.size() == 2);
            glm::vec3 mid0 = 0.5f * oldV.pos + 0.5f * oldVertices[oldV.b_vertex[0]].pos;
            glm::vec3 mid1 = 0.5f * oldV.pos + 0.5f * oldVertices[oldV.b_vertex[1]].pos;
            newV.pos = (oldV.pos + mid0 + mid1) / 3.0f;
        } else {
            newV.is_border = false;
            float n = (float)oldV.n_face.size();

            // 相邻面点的平均值
            glm::vec3 avgFP(0.0f);
            for (size_t j = 0; j < oldV.n_face.size(); j++) {
                avgFP += m_vertices[oldFaces[oldV.n_face[j]].fp_idx].pos;
            }
            avgFP /= n;

            // 相邻边中点的平均值
            glm::vec3 avgEM(0.0f);
            for (size_t j = 0; j < oldV.n_vertex.size(); j++) {
                avgEM += 0.5f * oldV.pos + 0.5f * oldVertices[oldV.n_vertex[j]].pos;
            }
            avgEM /= n;

            float m1 = (n - 3.0f) / n;
            float m2 = 1.0f / n;
            float m3 = 2.0f / n;
            newV.pos = m1 * oldV.pos + m2 * avgFP + m3 * avgEM;
        }
    }

    // ================================================================
    // 第四步: 生成新面
    // 三角形 → 3 个四边形, 四边形 → 4 个四边形
    // 每个新面: [原顶点, 边点1, 面点, 边点2]
    // ================================================================
    std::map<std::pair<int, int>, int> newEdgeIndex;

    for (size_t i = 0; i < oldFaces.size(); i++) {
        Face& oldFace = oldFaces[i];
        int fp_idx = oldFace.fp_idx;
        int n = oldFace.is_triangle ? 3 : 4;

        for (int j = 0; j < n; j++) {
            int ep1_idx, ep2_idx;

            if (j == 0) {
                ep1_idx = oldEdges[oldFace.n_edge[j]].ep_idx;
                ep2_idx = oldEdges[oldFace.n_edge[n - 1]].ep_idx;
            } else {
                ep1_idx = oldEdges[oldFace.n_edge[j]].ep_idx;
                ep2_idx = oldEdges[oldFace.n_edge[j - 1]].ep_idx;
            }

            Face newFace;
            unsigned int f_idx = (unsigned int)m_faces.size();

            newFace.v[0] = oldFace.v[j];
            newFace.v[1] = (unsigned int)ep1_idx;
            newFace.v[2] = (unsigned int)fp_idx;
            newFace.v[3] = (unsigned int)ep2_idx;
            newFace.is_triangle = false;

            // 为新面的顶点添加面引用
            for (int k = 0; k < 4; k++) {
                m_vertices[newFace.v[k]].n_face.push_back(f_idx);
            }

            // 生成/查找边
            for (int k = 0; k < 4; k++) {
                int vA = newFace.v[k];
                int vB = newFace.v[(k + 1) % 4];
                auto key = GetEdgeKey(vA, vB);

                unsigned int edgeIdx;
                if (newEdgeIndex.find(key) == newEdgeIndex.end()) {
                    edgeIdx = (unsigned int)m_edges.size();
                    Edge e;
                    e.v1 = key.first;
                    e.v2 = key.second;
                    m_edges.push_back(std::move(e));
                    newEdgeIndex[key] = (int)edgeIdx;
                    m_edges[edgeIdx].is_border = true;
                } else {
                    edgeIdx = newEdgeIndex[key];
                    m_edges[edgeIdx].is_border = false;
                }
                m_edges[edgeIdx].n_face.push_back(f_idx);
                newFace.n_edge.push_back(edgeIdx);
            }

            m_faces.push_back(newFace);
        }
    }

    // ---- 清理并重建邻接 ----
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].n_vertex.clear();
        m_vertices[i].b_vertex.clear();
    }
    BuildAdjacency();
}

// ============================================================
// ToMesh - 将内部数据转换为 Mesh
// ============================================================
Mesh CatmullClarkSubdivision::ToMesh()
{
    Mesh result;

    // 写入顶点位置
    for (size_t i = 0; i < m_vertices.size(); i++) {
        result.positions.push_back(m_vertices[i].pos.x);
        result.positions.push_back(m_vertices[i].pos.y);
        result.positions.push_back(m_vertices[i].pos.z);
    }

    // 写入面
    for (size_t i = 0; i < m_faces.size(); i++) {
        IndexedFace face;
        if (m_faces[i].is_triangle) {
            face.vertex_indices.push_back(m_faces[i].v[0]);
            face.vertex_indices.push_back(m_faces[i].v[1]);
            face.vertex_indices.push_back(m_faces[i].v[2]);
        } else {
            face.vertex_indices.push_back(m_faces[i].v[0]);
            face.vertex_indices.push_back(m_faces[i].v[1]);
            face.vertex_indices.push_back(m_faces[i].v[2]);
            face.vertex_indices.push_back(m_faces[i].v[3]);
        }
        result.faces.push_back(face);
    }

    return result;
}
