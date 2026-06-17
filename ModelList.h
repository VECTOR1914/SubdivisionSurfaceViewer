// ============================================================
// ModelList.h - 预置模型列表（Windows 资源引用）
// 模型数据通过 .rc 文件嵌入 exe，运行时通过 LoadFromResource 加载
// 单个 .exe 即可分发运行
// ============================================================
#pragma once

#include <cstddef>
#include "resource.h"

struct ModelEntry {
    const wchar_t* name;      // 显示名称
    int            resId;     // Windows 资源 ID
    unsigned int   sizeKB;    // 文件大小 (KB)
    bool           isLarge;   // true=大模型, false=小模型
};

// 所有预置模型（数据嵌入在 exe 的资源段中）
static const ModelEntry g_modelList[] = {
    // ---- 小模型 (< 500 KB) ----
    { L"pawn",      IDR_MODEL_PAWN,       49,  false },
    { L"deer",      IDR_MODEL_DEER,      103,  false },
    { L"cat",       IDR_MODEL_CAT,       228,  false },
    { L"teapot",    IDR_MODEL_TEAPOT,    264,  false },
    { L"salmon",    IDR_MODEL_SALMON,    282,  false },

    // ---- 大模型 (>= 500 KB) ----
    { L"dino",      IDR_MODEL_DINO,      600,  true },
    { L"vase",      IDR_MODEL_VASE,      672,  true },
    { L"spaceship", IDR_MODEL_SPACESHIP, 806,  true },
    { L"cow",       IDR_MODEL_COW,       910,  true },
    { L"face",      IDR_MODEL_FACE,     1074,  true },
    { L"dog",       IDR_MODEL_DOG,      1196,  true },
    { L"skeleton",  IDR_MODEL_SKELETON, 1522,  true },
    { L"ducky",     IDR_MODEL_DUCKY,    2256,  true },
    { L"bunny",     IDR_MODEL_BUNNY,    5213,  true },
    { L"bunny2",    IDR_MODEL_BUNNY2,   5227,  true },
    { L"catH",      IDR_MODEL_CATH,     6488,  true },
};

static const int g_modelListCount = sizeof(g_modelList) / sizeof(g_modelList[0]);
