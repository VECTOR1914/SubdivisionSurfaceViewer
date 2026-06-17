// ============================================================
// Mesh.cpp - OBJ 加载与保存实现
// ============================================================
#include "Mesh.h"
#include <cassert>
#include <cstring>
#include <iostream>

// ============================================================
// 辅助：判断是否为空白字符
// ============================================================
static inline bool isSpace(char c) {
    return (c == ' ') || (c == '\t');
}

static inline bool isNewLine(char c) {
    return (c == '\r') || (c == '\n') || (c == '\0');
}

// ============================================================
// 辅助：修复 OBJ 索引（支持负索引/相对索引）
// idx > 0  →  idx - 1  (OBJ 1-based → 0-based)
// idx < 0  →  n + idx  (相对索引)
// ============================================================
static inline int fixIndex(int idx, unsigned int n)
{
    if (idx > 0) {
        return idx - 1;
    } else if (idx == 0) {
        return 0;
    } else {
        // 负索引：从末尾倒数
        return (int)n + idx;
    }
}

// ============================================================
// LoadFromOBJ - 从文件加载 Wavefront OBJ
// 支持格式：v, f（含 v/vt/vn 格式）
// 返回 true 表示成功
// ============================================================
bool Mesh::LoadFromOBJ(const std::string& filepath)
{
    Clear();

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return false;
    }

    // ---- 第一遍：读取顶点 ----
    std::string line;
    while (std::getline(ifs, line)) {
        // 去除换行符
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty()) continue;

        const char* token = line.c_str();
        token += strspn(token, " \t");

        if (token[0] == '\0' || token[0] == '#') continue;

        // 顶点位置: v x y z
        if (token[0] == 'v' && isSpace(token[1])) {
            token += 2;
            token += strspn(token, " \t");
            float x = (float)atof(token);
            token += strcspn(token, " \t");
            token += strspn(token, " \t");
            float y = (float)atof(token);
            token += strcspn(token, " \t");
            token += strspn(token, " \t");
            float z = (float)atof(token);

            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
    }

    if (positions.empty()) {
        ifs.close();
        return false;
    }

    // 归一化顶点
    Normalize();

    // ---- 第二遍：读取面 ----
    ifs.clear();
    ifs.seekg(0, std::ios::beg);

    unsigned int numVertices = VertexCount();

    while (std::getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const char* token = line.c_str();
        token += strspn(token, " \t");

        if (token[0] == '\0' || token[0] == '#') continue;

        // 面: f v1 v2 v3 ... 或 f v1/vt1/vn1 v2/vt2/vn2 ...
        if (token[0] == 'f' && isSpace(token[1])) {
            token += 2;
            token += strspn(token, " \t");

            IndexedFace face;
            while (!isNewLine(token[0])) {
                // 解析 v_idx（忽略 vt 和 vn）
                int rawIdx = atoi(token);
                int v_idx = fixIndex(rawIdx, numVertices);

                // 跳过 /vt/vn 部分
                token += strcspn(token, " \t\r/");
                if (token[0] == '/') {
                    token++; // 跳过 /
                    // 跳过 vt
                    token += strcspn(token, " \t\r/");
                    if (token[0] == '/') {
                        token++; // 跳过 /
                        // 跳过 vn
                        token += strcspn(token, " \t\r");
                    }
                }

                if (v_idx >= 0 && v_idx < (int)numVertices) {
                    face.vertex_indices.push_back((unsigned int)v_idx);
                }

                token += strspn(token, " \t");
            }

            if (face.vertex_indices.size() >= 3) {
                faces.push_back(face);
            }
        }
    }

    ifs.close();
    return !faces.empty();
}

// ============================================================
// LoadFromMemory - 从内存字符串加载 OBJ
// 与 LoadFromOBJ 逻辑相同，仅将 ifstream 替换为 istringstream
// ============================================================
bool Mesh::LoadFromMemory(const char* data)
{
    Clear();

    if (!data || data[0] == '\0') return false;

    std::istringstream iss(data);

    // ---- 第一遍：读取顶点 ----
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const char* token = line.c_str();
        token += strspn(token, " \t");
        if (token[0] == '\0' || token[0] == '#') continue;

        if (token[0] == 'v' && isSpace(token[1])) {
            token += 2;
            token += strspn(token, " \t");
            float x = (float)atof(token);
            token += strcspn(token, " \t");
            token += strspn(token, " \t");
            float y = (float)atof(token);
            token += strcspn(token, " \t");
            token += strspn(token, " \t");
            float z = (float)atof(token);

            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
    }

    if (positions.empty()) return false;

    // 归一化顶点
    Normalize();

    // ---- 第二遍：读取面 ----
    iss.clear();
    iss.seekg(0, std::ios::beg);

    unsigned int numVertices = VertexCount();

    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const char* token = line.c_str();
        token += strspn(token, " \t");
        if (token[0] == '\0' || token[0] == '#') continue;

        if (token[0] == 'f' && isSpace(token[1])) {
            token += 2;
            token += strspn(token, " \t");

            IndexedFace face;
            while (!isNewLine(token[0])) {
                int rawIdx = atoi(token);
                int v_idx = fixIndex(rawIdx, numVertices);

                token += strcspn(token, " \t\r/");
                if (token[0] == '/') {
                    token++;
                    token += strcspn(token, " \t\r/");
                    if (token[0] == '/') {
                        token++;
                        token += strcspn(token, " \t\r");
                    }
                }

                if (v_idx >= 0 && v_idx < (int)numVertices) {
                    face.vertex_indices.push_back((unsigned int)v_idx);
                }

                token += strspn(token, " \t");
            }

            if (face.vertex_indices.size() >= 3) {
                faces.push_back(face);
            }
        }
    }

    return !faces.empty();
}

// ============================================================
// LoadFromResource - 从 Windows 资源 (RCDATA) 加载 OBJ
// 使用 FindResource → LoadResource → LockResource 获取数据
// 然后通过 LoadFromMemory 解析
// ============================================================
bool Mesh::LoadFromResource(HINSTANCE hInst, int resId)
{
    HRSRC hRes = FindResourceW(hInst, MAKEINTRESOURCEW(resId), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(hInst, hRes);
    if (!hData) return false;

    const char* rawData = (const char*)LockResource(hData);
    DWORD dataSize = SizeofResource(hInst, hRes);
    if (!rawData || dataSize == 0) return false;

    // 构造 null-terminated 字符串（资源数据不保证 null 结尾）
    std::string content(rawData, dataSize);
    return LoadFromMemory(content.c_str());
}

// ============================================================
// SaveToOBJ - 保存网格为 OBJ 文件
// ============================================================
bool Mesh::SaveToOBJ(const std::string& filepath) const
{
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return false;
    }

    // 写入顶点
    for (unsigned int i = 0; i < positions.size(); i += 3) {
        ofs << "v " << positions[i] << " "
                    << positions[i + 1] << " "
                    << positions[i + 2] << "\n";
    }

    // 写入面（OBJ 使用 1-based 索引）
    for (size_t i = 0; i < faces.size(); i++) {
        ofs << "f";
        for (size_t j = 0; j < faces[i].vertex_indices.size(); j++) {
            ofs << " " << (faces[i].vertex_indices[j] + 1);
        }
        ofs << "\n";
    }

    ofs.close();
    return true;
}
