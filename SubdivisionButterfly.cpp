// ============================================================
// SubdivisionButterfly.cpp - Butterfly 细分算法实现 (重写版)
//
// Modified Butterfly (Zorin et al. 1996)
// 参考: "Interpolating Subdivision for Meshes with Arbitrary Topology"
//
// 核心修复:
//   1. 用面-边邻接找到正确的 4 个外环顶点 (8 点模板)
//   2. 用面链构建顶点邻居的循环顺序 (Modified Butterfly 权重)
// ============================================================
#include "SubdivisionButterfly.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>
#include <set>

using namespace butterfly;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// LoadFromMesh
// ============================================================
bool SubdivisionButterfly::LoadFromMesh(const Mesh& mesh)
{
    m_vertices.clear(); m_edges.clear(); m_faces.clear();
    if (mesh.faces.empty() || mesh.positions.empty()) return false;

    std::map<std::pair<int, int>, int> edgeIndex;
    m_vertices.resize(mesh.VertexCount());

    for (unsigned int i = 0; i < mesh.faces.size(); i++) {
        const IndexedFace& curFace = mesh.faces[i];
        if (curFace.vertex_indices.size() != 3) {
            std::cout << "[Butterfly] 需要三角形网格" << std::endl;
            m_vertices.clear(); m_edges.clear(); m_faces.clear();
            return false;
        }
        unsigned int fi = (unsigned int)m_faces.size();
        Face f;
        for (unsigned int j = 0; j < 3; j++) {
            unsigned int vi = curFace.vertex_indices[j];
            f.v[j] = vi;
            float x, y, z; mesh.GetVertex(vi, x, y, z);
            m_vertices[vi].pos = glm::vec3(x, y, z);
            m_vertices[vi].n_face.push_back(fi);
        }
        for (unsigned int j = 0; j < 3; j++) {
            int a = (int)f.v[j], b = (int)f.v[(j + 1) % 3];
            auto key = std::pair<int,int>(std::min(a,b), std::max(a,b));
            unsigned int ei;
            if (edgeIndex.find(key) == edgeIndex.end()) {
                ei = (unsigned int)m_edges.size();
                Edge e; e.v1 = key.first; e.v2 = key.second;
                m_edges.push_back(e);
                edgeIndex[key] = (int)ei;
                m_edges[ei].is_border = true;
            } else {
                ei = edgeIndex[key];
                m_edges[ei].is_border = false;
            }
            m_edges[ei].n_face.push_back(fi);
            f.n_edge.push_back(ei);
        }
        m_faces.push_back(f);
    }

    BuildAdjacency();
    return true;
}

// ============================================================
// BuildAdjacency
// ============================================================
void SubdivisionButterfly::BuildAdjacency()
{
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].n_vertex.clear();
        m_vertices[i].b_vertex.clear();
    }
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
// GetCyclicNeighbors — 获取顶点周围邻居的循环顺序
//
// 通过遍历面链构建正确的环绕顺序:
//   对顶点 vi，它的每两个相邻面共享一条边，
//   这条边的另一端就是 vi 的一个邻居。
//   按面环绕 vi 的顺序就是邻居的循环顺序。
// ============================================================
static std::vector<unsigned int> GetCyclicNeighbors(
    unsigned int vi,
    const std::vector<Vertex>& verts,
    const std::vector<Face>& faces)
{
    const Vertex& v = verts[vi];
    int k = (int)v.n_face.size();
    if (k == 0) return {};

    // 用面链追踪顺序：从第一个面开始，通过共享边找下一个面
    std::vector<unsigned int> order;
    std::set<unsigned int> usedFaces;

    // 第一个面
    unsigned int curFace = v.n_face[0];
    usedFaces.insert(curFace);

    for (int step = 0; step < k; step++) {
        const Face& f = faces[curFace];
        // 找到 vi 在这个面中的位置，它的前后顶点是邻居
        int pos = -1;
        for (int j = 0; j < 3; j++)
            if (f.v[j] == vi) { pos = j; break; }

        // 顺时针方向的下一个邻居 (vi → f.v[(pos+1)%3])
        unsigned int nextNbr = f.v[(pos + 1) % 3];
        if (order.empty() || order.back() != nextNbr)
            order.push_back(nextNbr);

        // 找下一个面：共享边 (vi, nextNbr) 的面 (不是 curFace)
        unsigned int nextFace = curFace;
        for (unsigned int fi : verts[vi].n_face) {
            if (fi == curFace || usedFaces.count(fi)) continue;
            const Face& cand = faces[fi];
            bool sharesEdge = false;
            for (int j = 0; j < 3; j++)
                if (cand.v[j] == vi && cand.v[(j + 2) % 3] == nextNbr)
                    { sharesEdge = true; break; }
            // 也检查另一个方向
            for (int j = 0; j < 3; j++)
                if (cand.v[j] == nextNbr && cand.v[(j + 2) % 3] == vi)
                    { sharesEdge = true; break; }
            if (sharesEdge) { nextFace = fi; break; }
        }

        if (nextFace == curFace || usedFaces.count(nextFace)) break;
        usedFaces.insert(nextFace);
        curFace = nextFace;
    }

    return order;
}

// ============================================================
// FindEdgeIndex — 在边列表中查找指定顶点对的边索引
// ============================================================
static int FindEdgeIndex(unsigned int a, unsigned int b,
                          const std::vector<Edge>& edges)
{
    // 线性搜索 (顶点度数通常不大，性能可接受)
    for (size_t i = 0; i < edges.size(); i++) {
        if ((edges[i].v1 == a && edges[i].v2 == b) ||
            (edges[i].v1 == b && edges[i].v2 == a))
            return (int)i;
    }
    return -1;
}

// ============================================================
// ModifiedButterflyWeight
// ============================================================
float SubdivisionButterfly::ModifiedButterflyWeight(int k, int j) const
{
    if (k == 3) {
        float w[3] = { 5.0f/12.0f, -1.0f/12.0f, -1.0f/12.0f };
        return w[j % 3];
    }
    if (k == 4) {
        float w[4] = { 3.0f/8.0f, 0.0f, -1.0f/8.0f, 0.0f };
        return w[j % 4];
    }
    float angle = 2.0f * (float)M_PI * (float)j / (float)k;
    return (1.0f / (float)k) * (0.25f + cosf(angle) + 0.5f * cosf(2.0f * angle));
}

// ============================================================
// SubdivideOnce — 执行一次 Butterfly 细分 (重写)
// ============================================================
void SubdivisionButterfly::SubdivideOnce()
{
    std::vector<Vertex> oldVertices(m_vertices);
    std::vector<Edge>   oldEdges(m_edges);
    std::vector<Face>   oldFaces(m_faces);

    m_vertices.clear(); m_edges.clear(); m_faces.clear();

    // ---- 第一步: 复制旧顶点 (Butterfly 是插值型) ----
    m_vertices.resize(oldVertices.size());
    for (size_t i = 0; i < oldVertices.size(); i++) {
        m_vertices[i].pos = oldVertices[i].pos;
        m_vertices[i].is_border = oldVertices[i].is_border;
    }

    // ---- 预计算每个顶点的循环邻居顺序 ----
    std::vector<std::vector<unsigned int>> cyclicOrder(oldVertices.size());
    for (size_t vi = 0; vi < oldVertices.size(); vi++) {
        if (!oldVertices[vi].is_border)
            cyclicOrder[vi] = GetCyclicNeighbors((unsigned int)vi, oldVertices, oldFaces);
    }

    // ---- 第二步: 为每条边计算新顶点 (边点) ----
    for (size_t ei = 0; ei < oldEdges.size(); ei++) {
        Edge& curEdge = oldEdges[ei];
        unsigned int v0i = curEdge.v1, v1i = curEdge.v2;
        const Vertex& v0 = oldVertices[v0i];
        const Vertex& v1 = oldVertices[v1i];
        Vertex mid;
        unsigned int midIdx = (unsigned int)m_vertices.size();

        if (curEdge.is_border) {
            mid.pos = 0.5f * (v0.pos + v1.pos);
            mid.is_border = true;
        } else {
            const Face& f0 = oldFaces[curEdge.n_face[0]];
            const Face& f1 = oldFaces[curEdge.n_face[1]];

            // 对立顶点
            unsigned int opp0 = 0, opp1 = 0;
            for (int k = 0; k < 3; k++) {
                if (f0.v[k] != v0i && f0.v[k] != v1i) opp0 = f0.v[k];
                if (f1.v[k] != v0i && f1.v[k] != v1i) opp1 = f1.v[k];
            }

            // ---- 规则情况 (both valence 6): 标准 8 点蝶形模板 ----
            if (cyclicOrder[v0i].size() == 6 && cyclicOrder[v1i].size() == 6) {
                // 通过面-边邻接找到 4 个外环顶点
                // face0 侧: 边(v0,opp0)和(v1,opp0)的另一个共享面 → 第三个顶点
                // face1 侧: 边(v0,opp1)和(v1,opp1)的另一个共享面 → 第三个顶点
                std::vector<unsigned int> outer;
                unsigned int edgePairs[4][2] = {
                    {v0i, opp0}, {v1i, opp0}, {v0i, opp1}, {v1i, opp1}
                };
                for (int p = 0; p < 4; p++) {
                    int eidx = FindEdgeIndex(edgePairs[p][0], edgePairs[p][1], oldEdges);
                    if (eidx >= 0) {
                        const Edge& sharedEdge = oldEdges[eidx];
                        for (unsigned int fi : sharedEdge.n_face) {
                            if (fi != curEdge.n_face[0] && fi != curEdge.n_face[1]) {
                                const Face& outerFace = oldFaces[fi];
                                for (int k = 0; k < 3; k++) {
                                    unsigned int w = outerFace.v[k];
                                    if (w != edgePairs[p][0] && w != edgePairs[p][1]) {
                                        outer.push_back(w);
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }

                if (outer.size() >= 4) {
                    mid.pos = 0.5f * (v0.pos + v1.pos)
                            + 0.125f * (oldVertices[opp0].pos + oldVertices[opp1].pos)
                            - 0.0625f * (oldVertices[outer[0]].pos + oldVertices[outer[1]].pos
                                       + oldVertices[outer[2]].pos + oldVertices[outer[3]].pos);
                } else {
                    // 外环不足 → 退化为 Loop 边点公式
                    mid.pos = 0.375f * (v0.pos + v1.pos)
                            + 0.125f * (oldVertices[opp0].pos + oldVertices[opp1].pos);
                }
                mid.is_border = false;
            } else {
                // ---- 奇异情况: Modified Butterfly ----
                // 使用预先计算好的循环顺序

                // 从 v0 的角度: 邻居循环顺序中，v1 是参考点 (j=0)
                const auto& order0 = cyclicOrder[v0i];
                int k0 = (int)order0.size();
                // 在循环顺序中找到 v1、opp0、opp1 的位置
                int j_v1 = -1, j_opp0 = -1, j_opp1 = -1;
                for (int j = 0; j < k0; j++) {
                    if (order0[j] == v1i)  j_v1 = j;
                    if (order0[j] == opp0) j_opp0 = j;
                    if (order0[j] == opp1) j_opp1 = j;
                }

                glm::vec3 pos0(0.0f);
                if (k0 > 0 && j_v1 >= 0) {
                    float sumW = 0.0f;
                    for (int j = 0; j < k0; j++) {
                        // 以 v1 为 j=0，计算各邻居的循环距离
                        int dist = (j - j_v1 + k0) % k0;
                        float w = ModifiedButterflyWeight(k0, dist);
                        pos0 += w * oldVertices[order0[j]].pos;
                        sumW += w;
                    }
                    pos0 += (1.0f - sumW) * v0.pos;
                } else {
                    pos0 = v0.pos;
                }

                // 从 v1 的角度
                const auto& order1 = cyclicOrder[v1i];
                int k1 = (int)order1.size();
                int j2_v0 = -1;
                for (int j = 0; j < k1; j++) {
                    if (order1[j] == v0i) j2_v0 = j;
                }

                glm::vec3 pos1(0.0f);
                if (k1 > 0 && j2_v0 >= 0) {
                    float sumW = 0.0f;
                    for (int j = 0; j < k1; j++) {
                        int dist = (j - j2_v0 + k1) % k1;
                        float w = ModifiedButterflyWeight(k1, dist);
                        pos1 += w * oldVertices[order1[j]].pos;
                        sumW += w;
                    }
                    pos1 += (1.0f - sumW) * v1.pos;
                } else {
                    pos1 = v1.pos;
                }

                mid.pos = 0.5f * (pos0 + pos1);
                mid.is_border = false;
            }
        }

        m_vertices.push_back(std::move(mid));
        curEdge.mid = midIdx;
    }

    // ---- 第三步: 生成新面 (同 Loop 拓扑) ----
    m_edges.resize(oldEdges.size() * 2);
    m_faces.resize(oldFaces.size() * 4);

    for (unsigned int fi = 0; fi < oldFaces.size(); fi++) {
        const Face& cur = oldFaces[fi];
        const Edge& e0 = oldEdges[cur.n_edge[0]];
        const Edge& e1 = oldEdges[cur.n_edge[1]];
        const Edge& e2 = oldEdges[cur.n_edge[2]];
        unsigned int mid[3] = { e0.mid, e1.mid, e2.mid };

        unsigned int innerIdx[3];
        for (int j = 0; j < 3; j++) {
            Edge ie;
            ie.v1 = mid[j]; ie.v2 = mid[(j + 2) % 3];
            ie.n_face.push_back(4*fi + j); ie.n_face.push_back(4*fi + 3);
            ie.is_border = false;
            innerIdx[j] = (unsigned int)m_edges.size();
            m_edges.push_back(ie);
        }

        unsigned int outerIdx[3][2];
        for (int j = 0; j < 3; j++) {
            const Edge& oldE = oldEdges[cur.n_edge[j]];
            // 用旧边的规范顶点顺序 (v1<v2) 判断方向：
            // 子边 0 = 较小顶点→中点, 子边 1 = 中点→较大顶点
            // 如果当前面的边起点恰好是较小顶点，则 k=0→子边0, k=1→子边1
            // 否则对调
            bool startsWithSmaller = (cur.v[j] == oldE.v1 && cur.v[(j+1)%3] == oldE.v2);
            int k0 = startsWithSmaller ? 0 : 1;
            int k1 = startsWithSmaller ? 1 : 0;

            unsigned int base0 = 2 * cur.n_edge[j] + k0;
            Edge& oe0 = m_edges[base0];
            oe0.v1 = cur.v[j]; oe0.v2 = mid[j];
            oe0.is_border = oldE.is_border;
            outerIdx[j][0] = base0;

            unsigned int base1 = 2 * cur.n_edge[j] + k1;
            Edge& oe1 = m_edges[base1];
            oe1.v1 = mid[j]; oe1.v2 = cur.v[(j+1)%3];
            oe1.is_border = oldE.is_border;
            outerIdx[j][1] = base1;
        }

        for (int j = 0; j < 3; j++) {
            Face& f = m_faces[4*fi + j];
            f.v[0] = cur.v[j]; f.v[1] = mid[j]; f.v[2] = mid[(j+2)%3];
            f.n_edge.push_back(outerIdx[j][0]);
            f.n_edge.push_back(innerIdx[j]);
            f.n_edge.push_back(outerIdx[(j+2)%3][1]);
            m_edges[outerIdx[j][0]].n_face.push_back(4*fi + j);
            m_edges[outerIdx[(j+2)%3][1]].n_face.push_back(4*fi + j);
        }

        Face& innerF = m_faces[4*fi + 3];
        for (int j = 0; j < 3; j++) {
            innerF.v[j] = mid[j];
            innerF.n_edge.push_back(innerIdx[(j+1)%3]);
        }
    }

    // ---- 重建邻接 ----
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].n_vertex.clear();
        m_vertices[i].b_vertex.clear();
        m_vertices[i].n_face.clear();
        m_vertices[i].is_border = false;
    }
    BuildAdjacency();
    for (unsigned int fi = 0; fi < m_faces.size(); fi++)
        for (int j = 0; j < 3; j++)
            m_vertices[m_faces[fi].v[j]].n_face.push_back(fi);
}

// ============================================================
// ToMesh
// ============================================================
Mesh SubdivisionButterfly::ToMesh()
{
    Mesh result;
    for (size_t i = 0; i < m_vertices.size(); i++) {
        result.positions.push_back(m_vertices[i].pos.x);
        result.positions.push_back(m_vertices[i].pos.y);
        result.positions.push_back(m_vertices[i].pos.z);
    }
    for (size_t i = 0; i < m_faces.size(); i++) {
        IndexedFace face;
        face.vertex_indices.push_back(m_faces[i].v[0]);
        face.vertex_indices.push_back(m_faces[i].v[1]);
        face.vertex_indices.push_back(m_faces[i].v[2]);
        result.faces.push_back(face);
    }
    return result;
}
