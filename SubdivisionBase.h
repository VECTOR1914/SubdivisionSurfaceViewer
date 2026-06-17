// ============================================================
// SubdivisionBase.h - 细分曲面算法抽象基类
// 所有细分算法（Loop, Catmull-Clark, Doo-Sabin）的公共接口
// ============================================================
#ifndef SubdivisionBase_H
#define SubdivisionBase_H

#include "Mesh.h"

class SubdivisionBase
{
public:
    virtual ~SubdivisionBase() {}

    // 从 Mesh 加载数据，构建内部邻接结构
    // 返回 false 表示网格类型不兼容（如 Loop 需要三角形）
    virtual bool LoadFromMesh(const Mesh& mesh) = 0;

    // 执行一次细分
    virtual void SubdivideOnce() = 0;

    // 将内部数据转换为 Mesh
    virtual Mesh ToMesh() = 0;

    // 执行 N 次细分
    Mesh Execute(unsigned int times)
    {
        for (unsigned int i = 0; i < times; i++) {
            SubdivideOnce();
        }
        return ToMesh();
    }

    // 获取内部顶点/面数（用于调试/信息显示）
    virtual unsigned int GetVertexCount() const = 0;
    virtual unsigned int GetFaceCount() const = 0;
};

#endif // SubdivisionBase_H
