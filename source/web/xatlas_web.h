#ifndef _XATLAS_WEB_H_
#define _XATLAS_WEB_H_

#include <stdlib.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include "../xatlas/xatlas.h"

extern "C" {
    extern void onAtlasProgress(int mode, int progress);
}

struct MeshBufferInfo {
    uint32_t meshId;
    uint32_t indexOffset;
    uint32_t positionOffset;
    uint32_t normalOffset;
    uint32_t uvOffset;
};

struct UvMeshBufferInfo {
    uint32_t meshId;
    uint32_t indexOffset;
    uint32_t uvOffset;
};

struct AtlasMeshBufferInfo {
    uint32_t newVertexCount;
    uint32_t newIndexCount;
    uint32_t indexOffset;
    uint32_t originalIndexOffset;
    uint32_t uvOffset;
};

extern void createAtlas();

extern MeshBufferInfo createMesh(uint32_t vertexCount, uint32_t indexCount, bool normals, bool uvs);

extern UvMeshBufferInfo createUvMesh(int vertexCount, int indexCount);

extern uint32_t addMesh();

extern uint32_t addUvMesh();

extern xatlas::ChartOptions defaultChartOptions();
extern xatlas::PackOptions defaultPackOptions();

extern void computeCharts(const xatlas::ChartOptions chartOptions);
extern void packCharts(const xatlas::PackOptions packOptions);

extern void generateAtlas(const xatlas::ChartOptions chartOptions, const xatlas::PackOptions packOptions);

extern AtlasMeshBufferInfo getMeshData(uint32_t meshId);

extern void destroyMeshData(const AtlasMeshBufferInfo atlasMeshBufferInfo);

extern void destroyAtlas();

extern void setProgressLogging(bool f); // bool to enable/disable pretty progress logging.

#endif