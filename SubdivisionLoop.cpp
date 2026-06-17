// ============================================================
// SubdivisionLoop.cpp - Loop 细分算法实现
// 改编自 Sample project/Mesh-Subdivisions/src/subdivisions/loop.cpp
// ============================================================
#include "SubdivisionLoop.h"
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace loop;

// ============================================================
// LoadFromMesh - 从 Mesh 构建邻接数据结构
// 要求输入为三角形网格（非三角形面需预先调用 Mesh::Triangulate()）
// ============================================================
bool LoopSubdivision::LoadFromMesh(const Mesh& mesh)
{
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();

    if (mesh.faces.empty() || mesh.positions.empty())
        return false;

    std::map<std::pair<int, int>, int> edgeIndex; // 边去重（小索引, 大索引）
    m_vertices.resize(mesh.VertexCount());

    // ---- 遍历所有面 ----
    for (unsigned int i = 0; i < mesh.faces.size(); i++) {
        const IndexedFace& curFace = mesh.faces[i];
        unsigned int faceIdx = (unsigned int)m_faces.size();
        Face f;

        // Loop 细分要求三角形
        if (curFace.vertex_indices.size() != 3) {
            std::cout << "[Loop] 警告: 网格包含非三角形面，请先调用 Mesh::Triangulate()" << std::endl;
            m_vertices.clear(); m_edges.clear(); m_faces.clear();
            return false;
        }

        // ---- 构建 3 个顶点 ----
        for (unsigned int j = 0; j < 3; j++) {
            unsigned int vi = curFace.vertex_indices[j];
            f.v[j] = vi;

            // 读取位置
            float x, y, z;
            mesh.GetVertex(vi, x, y, z);
            m_vertices[vi].pos = glm::vec3(x, y, z);
            m_vertices[vi].n_face.push_back(faceIdx);
            m_vertices[vi].is_border = false;
        }

        // ---- 构建 3 条边 ----
        for (unsigned int j = 0; j < 3; j++) {
            unsigned int edgeIdx;
            int mini, maxi;
            if (j != 2) {
                mini = std::min((int)f.v[j], (int)f.v[j + 1]);
                maxi = std::max((int)f.v[j], (int)f.v[j + 1]);
            } else {
                mini = std::min((int)f.v[2], (int)f.v[0]);
                maxi = std::max((int)f.v[2], (int)f.v[0]);
            }
            auto key = std::pair<int, int>(mini, maxi);

            if (edgeIndex.find(key) == edgeIndex.end()) {
                // 新边
                edgeIdx = (int)m_edges.size();
                Edge e;
                e.v1 = key.first;
                e.v2 = key.second;
                m_edges.push_back(std::move(e));
                edgeIndex[key] = (int)edgeIdx;
                m_edges[edgeIdx].is_border = true;
            } else {
                // 已有边（非边界）
                edgeIdx = edgeIndex[key];
                m_edges[edgeIdx].is_border = false;
            }
            m_edges[edgeIdx].n_face.push_back(faceIdx);
            f.n_edge.push_back(edgeIdx);
        }

        m_faces.push_back(f);
    }

    // ---- 构建顶点邻接关系 ----
    BuildAdjacency();

    return true;
}

// ============================================================
// BuildAdjacency - 从边数据构建顶点之间的邻接关系
// ============================================================
void LoopSubdivision::BuildAdjacency()
{
    for (size_t i = 0; i < m_edges.size(); i++) {
        const Edge& e = m_edges[i];
        if (e.is_border) {
            // 边界边：两端顶点互为边界邻居
            m_vertices[e.v1].is_border = true;
            m_vertices[e.v2].is_border = true;
            m_vertices[e.v1].b_vertex.push_back(e.v2);
            m_vertices[e.v2].b_vertex.push_back(e.v1);
        } else {
            // 内部边：两端顶点互为邻居
            m_vertices[e.v1].n_vertex.push_back(e.v2);
            m_vertices[e.v2].n_vertex.push_back(e.v1);
        }
    }
}

// ============================================================
// SubdivideOnce - 执行一次 Loop 细分
// ============================================================
void LoopSubdivision::SubdivideOnce()
{
    // 保存旧数据
    std::vector<Vertex> oldVertices(m_vertices);
    std::vector<Edge>   oldEdges(m_edges);
    std::vector<Face>   oldFaces(m_faces);

    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
    m_vertices.resize(oldVertices.size());

    // ================================================================
    // 第一步: 为每条边生成新顶点（边点），放在旧顶点之后
    // ================================================================
    for (size_t ei = 0; ei < oldEdges.size(); ei++) {
        Edge& curEdge = oldEdges[ei];
        const Vertex& v0 = oldVertices[curEdge.v1];
        const Vertex& v1 = oldVertices[curEdge.v2];
        Vertex mid;
        int midIdx = (int)m_vertices.size();

        if (curEdge.is_border) {
            // 边界边：中点
            mid.pos = 0.5f * v0.pos + 0.5f * v1.pos;
            mid.is_border = true;
        } else {
            // 内部边：3/8(v0+v1) + 1/8(v2+v3)
            // v2, v3 是共享此边的两个面的第三个顶点
            const Face& f0 = oldFaces[curEdge.n_face[0]];
            const Face& f1 = oldFaces[curEdge.n_face[1]];

            unsigned int farIdx0 = 0, farIdx1 = 0;
            for (unsigned int k = 0; k < 3; k++) {
                if (f0.v[k] != curEdge.v1 && f0.v[k] != curEdge.v2) farIdx0 = f0.v[k];
                if (f1.v[k] != curEdge.v1 && f1.v[k] != curEdge.v2) farIdx1 = f1.v[k];
            }

            const Vertex& v2 = oldVertices[farIdx0];
            const Vertex& v3 = oldVertices[farIdx1];

            mid.pos = 0.375f * v0.pos + 0.375f * v1.pos +
                      0.125f * v2.pos + 0.125f * v3.pos;
            mid.is_border = false;
        }

        m_vertices.push_back(std::move(mid));
        curEdge.mid = midIdx; // 暂存于旧边中
    }

    // ================================================================
    // 第二步: 更新原始顶点位置
    // ================================================================
    for (unsigned int vi = 0; vi < oldVertices.size(); vi++) {
        const Vertex& cur = oldVertices[vi];

        if (cur.is_border) {
            // 边界顶点: 3/4*v + 1/8*n0 + 1/8*n1
            m_vertices[vi].pos = 0.75f * cur.pos +
                0.125f * oldVertices[cur.b_vertex[0]].pos +
                0.125f * oldVertices[cur.b_vertex[1]].pos;
            m_vertices[vi].is_border = true;
        } else {
            // 内部顶点: (1 - n*β)*v + β*Σneighbors
            unsigned int k = (unsigned int)cur.n_vertex.size();
            float beta = 1.0f / k * (0.625f - powf(0.375f + 0.25f * cosf(2.0f * 3.14159265359f / k), 2.0f));

            glm::vec3 pos = (1.0f - k * beta) * cur.pos;
            for (unsigned int j = 0; j < k; j++) {
                pos += beta * oldVertices[cur.n_vertex[j]].pos;
            }
            m_vertices[vi].pos = pos;
            m_vertices[vi].is_border = false;
        }
    }

    // ================================================================
    // 第三步: 生成新面（每旧面 → 4 个新三角形面）
    // 外圈 3 个面 + 中心 1 个面
    // ================================================================
    m_edges.resize(oldEdges.size() * 2);
    m_faces.resize(oldFaces.size() * 4);

    for (unsigned int fi = 0; fi < oldFaces.size(); fi++) {
        const Face& cur = oldFaces[fi];
        const Edge& e0 = oldEdges[cur.n_edge[0]];
        const Edge& e1 = oldEdges[cur.n_edge[1]];
        const Edge& e2 = oldEdges[cur.n_edge[2]];

        unsigned int mid[3] = { e0.mid, e1.mid, e2.mid };

        // ---- 构建 3 条内部新边（连接 3 个中点）----
        unsigned int innerEdgeIdx[3];
        for (unsigned int j = 0; j < 3; j++) {
            Edge innerEdge;
            innerEdge.v1 = mid[j];
            innerEdge.v2 = mid[(j + 2) % 3]; // 连接 mid[0]-mid[2], mid[1]-mid[0], mid[2]-mid[1]
            innerEdge.n_face.push_back(4 * fi + j);
            innerEdge.n_face.push_back(4 * fi + 3);
            innerEdge.is_border = false;
            innerEdgeIdx[j] = (int)m_edges.size();
            m_edges.push_back(innerEdge);
        }

        // ---- 构建 6 条外部新边（原边一分为二）----
        // 用旧边的规范顶点顺序 (v1<v2) 确保两个共享面写入一致的子边索引
        unsigned int outerEdgeIdx[3][2];
        for (unsigned int j = 0; j < 3; j++) {
            const Edge& oldE = oldEdges[cur.n_edge[j]];
            // 当前面从 cur.v[j]→cur.v[(j+1)%3] 经过此边
            // 判断面的方向是否与边的规范方向 (v1→v2) 一致
            bool faceGoesSmallToLarge = (cur.v[j] == oldE.v1 && cur.v[(j+1)%3] == oldE.v2);
            int k0 = faceGoesSmallToLarge ? 0 : 1;
            int k1 = faceGoesSmallToLarge ? 1 : 0;

            // 子边 k0 (小→中): 从较小端点出发，到中点
            unsigned int baseSmall = 2 * cur.n_edge[j] + k0;
            m_edges[baseSmall].v1 = cur.v[j];
            m_edges[baseSmall].v2 = mid[j];
            m_edges[baseSmall].is_border = oldE.is_border;
            outerEdgeIdx[j][0] = baseSmall;

            // 子边 k1 (中→大): 从中点出发，到较大端点
            unsigned int baseLarge = 2 * cur.n_edge[j] + k1;
            m_edges[baseLarge].v1 = mid[j];
            m_edges[baseLarge].v2 = cur.v[(j+1)%3];
            m_edges[baseLarge].is_border = oldE.is_border;
            outerEdgeIdx[j][1] = baseLarge;
        }

        // ---- 构建 3 个外圈新面 ----
        for (unsigned int j = 0; j < 3; j++) {
            Face& f = m_faces[4 * fi + j];
            f.v[0] = cur.v[j];
            f.v[1] = mid[j];
            f.v[2] = mid[(j + 2) % 3];

            f.n_edge.push_back(outerEdgeIdx[j][0]);          // 原顶点→中点
            f.n_edge.push_back(innerEdgeIdx[j]);             // 内部边
            f.n_edge.push_back(outerEdgeIdx[(j + 2) % 3][1]); // 中点→原顶点

            // 设置外部边的面引用
            m_edges[outerEdgeIdx[j][0]].n_face.push_back(4 * fi + j);
            m_edges[outerEdgeIdx[(j + 2) % 3][1]].n_face.push_back(4 * fi + j);
        }

        // ---- 构建中心新面 ----
        Face& innerFace = m_faces[4 * fi + 3];
        for (unsigned int j = 0; j < 3; j++) {
            innerFace.v[j] = mid[j];
            // 内部边按逆时针: innerEdge[1], innerEdge[2], innerEdge[0]
            innerFace.n_edge.push_back(innerEdgeIdx[(j + 1) % 3]);
        }
    }

    // ---- 重建顶点邻接关系 ----
    // 先清除旧的邻接
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].n_vertex.clear();
        m_vertices[i].b_vertex.clear();
        m_vertices[i].n_face.clear();
        m_vertices[i].is_border = false;
    }

    // 从边数据重建邻接
    BuildAdjacency();

    // 从面数据填充顶点的 n_face
    for (unsigned int fi = 0; fi < m_faces.size(); fi++) {
        for (unsigned int j = 0; j < 3; j++) {
            m_vertices[m_faces[fi].v[j]].n_face.push_back(fi);
        }
    }
}

// ============================================================
// ToMesh - 将内部数据转换为 Mesh
// ============================================================
Mesh LoopSubdivision::ToMesh()
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
        face.vertex_indices.push_back(m_faces[i].v[0]);
        face.vertex_indices.push_back(m_faces[i].v[1]);
        face.vertex_indices.push_back(m_faces[i].v[2]);
        result.faces.push_back(face);
    }

    return result;
}
