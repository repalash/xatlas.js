#include "xatlas_web.h"

xatlas::Atlas *atlas;
xatlas::MeshDecl *meshDecl;
xatlas::UvMeshDecl *uvMeshDecl;
uint32_t nextMeshId = 0;

void createAtlas() {
    atlas = xatlas::Create();
}

MeshBufferInfo createMesh(uint32_t vertexCount, uint32_t indexCount, bool normals, bool uvs) {
    MeshBufferInfo meshBufferInfo;
    meshBufferInfo.meshId = nextMeshId++;

    meshDecl = new xatlas::MeshDecl;
    meshDecl->vertexCount = vertexCount;
    meshDecl->indexCount = indexCount;

    meshDecl->indexData = new uint16_t[indexCount];
    meshDecl->indexFormat = xatlas::IndexFormat::UInt16;
    meshBufferInfo.indexOffset = (uint32_t) meshDecl->indexData;

    meshDecl->vertexPositionData = new float[vertexCount * 3];
    meshDecl->vertexPositionStride = sizeof(float) * 3;
    meshBufferInfo.positionOffset = (uint32_t) meshDecl->vertexPositionData;

    if (normals) {
        meshDecl->vertexNormalData = new float[vertexCount * 3];
        meshDecl->vertexNormalStride = sizeof(float) * 3;
        meshBufferInfo.normalOffset = (uint32_t) meshDecl->vertexNormalData;
    }

    if (uvs) {
        meshDecl->vertexUvData = new float[vertexCount * 2];
        meshDecl->vertexUvStride = sizeof(float) * 2;
        meshBufferInfo.uvOffset = (uint32_t) meshDecl->vertexUvData;
    }

    return meshBufferInfo;
}

UvMeshBufferInfo createUvMesh(int vertexCount, int indexCount) {
    UvMeshBufferInfo uvMeshBufferInfo;
    uvMeshBufferInfo.meshId = nextMeshId++;

    uvMeshDecl = new xatlas::UvMeshDecl;
    uvMeshDecl->vertexCount = vertexCount;
    uvMeshDecl->indexCount = indexCount;

    uvMeshDecl->indexData = new uint16_t[indexCount];
    uvMeshDecl->indexFormat = xatlas::IndexFormat::UInt16;
    uvMeshBufferInfo.indexOffset = (uint32_t) uvMeshDecl->indexData;

    uvMeshDecl->vertexUvData = new float[vertexCount * 2];
    uvMeshDecl->vertexStride = sizeof(float) * 2;
    uvMeshBufferInfo.uvOffset = (uint32_t) uvMeshDecl->vertexUvData;

    return uvMeshBufferInfo;
}

uint32_t addMesh() {
    if (meshDecl == nullptr) return 5;
    uint32_t mesh = (uint32_t) xatlas::AddMesh(atlas, *meshDecl);
    delete[] (float *) meshDecl->vertexUvData;
    delete[] (float *) meshDecl->vertexNormalData;
    delete[] (float *) meshDecl->vertexPositionData;
    delete[] (uint16_t *) meshDecl->indexData;
    delete meshDecl;
    meshDecl = nullptr;
    return mesh;
}

uint32_t addUvMesh() {
    if (uvMeshDecl == nullptr) return 5;
    uint32_t mesh = (uint32_t) xatlas::AddUvMesh(atlas, *uvMeshDecl);
    delete[] (float *) uvMeshDecl->vertexUvData;
    delete[] (uint16_t *) uvMeshDecl->indexData;
    delete uvMeshDecl;
    uvMeshDecl = nullptr;
    return mesh;
}

void generateAtlas(const xatlas::ChartOptions chartOptions = defaultChartOptions(), const xatlas::PackOptions packOptions=defaultPackOptions()) {
    xatlas::Generate(atlas, chartOptions, packOptions);
}

xatlas::ChartOptions defaultChartOptions(){
    return xatlas::ChartOptions();
}

xatlas::PackOptions defaultPackOptions(){
    return xatlas::PackOptions();
}

AtlasMeshBufferInfo getMeshData(uint32_t meshId) {
    const xatlas::Mesh &mesh = atlas->meshes[meshId];

    uint32_t *originalIndexArray = new uint32_t[mesh.vertexCount];
    float *uvArray = new float[mesh.vertexCount * 2];

    for (uint32_t i = 0; i < mesh.vertexCount; i++) {
        const xatlas::Vertex &vertex = mesh.vertexArray[i];
        originalIndexArray[i] = vertex.xref;
        uvArray[i * 2] = vertex.uv[0] / atlas->width;
        uvArray[i * 2 + 1] = vertex.uv[1] / atlas->height;
    }

    AtlasMeshBufferInfo atlasMeshBufferInfo;

    atlasMeshBufferInfo.newVertexCount = mesh.vertexCount;
    atlasMeshBufferInfo.newIndexCount = mesh.indexCount;
    atlasMeshBufferInfo.indexOffset = (uint32_t) mesh.indexArray;
    atlasMeshBufferInfo.originalIndexOffset = (uint32_t) originalIndexArray;
    atlasMeshBufferInfo.uvOffset = (uint32_t) uvArray;

    return atlasMeshBufferInfo;
}

void destroyMeshData(const AtlasMeshBufferInfo atlasMeshBufferInfo) {
    delete[] (uint32_t *) atlasMeshBufferInfo.originalIndexOffset;
    delete[] (float *) atlasMeshBufferInfo.uvOffset;
}

void destroyAtlas() {
    xatlas::Destroy(atlas);
    nextMeshId = 0;
}

#ifdef SANITIZE_ADDRESS_CHECK
#include <sanitizer/lsan_interface.h>
#else
void __lsan_do_recoverable_leak_check() {}
#endif

EMSCRIPTEN_BINDINGS(xatlas) {
        emscripten::value_object<MeshBufferInfo>("MeshBufferInfo")
                .field("meshId", &MeshBufferInfo::meshId)
                .field("indexOffset", &MeshBufferInfo::indexOffset)
                .field("positionOffset", &MeshBufferInfo::positionOffset)
                .field("normalOffset", &MeshBufferInfo::normalOffset)
                .field("uvOffset", &MeshBufferInfo::uvOffset);

        emscripten::value_object<UvMeshBufferInfo>("UvMeshBufferInfo")
        .field("meshId", &UvMeshBufferInfo::meshId)
        .field("indexOffset", &UvMeshBufferInfo::indexOffset)
        .field("uvOffset", &UvMeshBufferInfo::uvOffset);

        emscripten::value_object<AtlasMeshBufferInfo>("AtlasMeshBufferInfo")
        .field("newVertexCount", &AtlasMeshBufferInfo::newVertexCount)
        .field("newIndexCount", &AtlasMeshBufferInfo::newIndexCount)
        .field("indexOffset", &AtlasMeshBufferInfo::indexOffset)
        .field("originalIndexOffset", &AtlasMeshBufferInfo::originalIndexOffset)
        .field("uvOffset", &AtlasMeshBufferInfo::uvOffset);

        emscripten::value_object<xatlas::ChartOptions>("ChartOptions")
        .field("maxChartArea", &xatlas::ChartOptions::maxChartArea)
        .field("maxBoundaryLength", &xatlas::ChartOptions::maxBoundaryLength)
        .field("normalDeviationWeight", &xatlas::ChartOptions::normalDeviationWeight)
        .field("roundnessWeight", &xatlas::ChartOptions::roundnessWeight)
        .field("straightnessWeight", &xatlas::ChartOptions::straightnessWeight)
        .field("normalSeamWeight", &xatlas::ChartOptions::normalSeamWeight)
        .field("textureSeamWeight", &xatlas::ChartOptions::textureSeamWeight)
        .field("maxCost", &xatlas::ChartOptions::maxCost)
        .field("maxIterations", &xatlas::ChartOptions::maxIterations)
        .field("useInputMeshUvs", &xatlas::ChartOptions::maxIterations)
        .field("fixWinding", &xatlas::ChartOptions::maxIterations);

        emscripten::value_object<xatlas::PackOptions>("PackOptions")
        .field("bilinear", &xatlas::PackOptions::bilinear)
        .field("blockAlign", &xatlas::PackOptions::blockAlign)
        .field("bruteForce", &xatlas::PackOptions::bruteForce)
        .field("createImage", &xatlas::PackOptions::createImage)
        .field("maxChartSize", &xatlas::PackOptions::maxChartSize)
        .field("padding", &xatlas::PackOptions::padding)
        .field("texelsPerUnit", &xatlas::PackOptions::texelsPerUnit)
        .field("resolution", &xatlas::PackOptions::resolution);

        emscripten::function("createAtlas", &createAtlas);
        emscripten::function("createMesh", &createMesh);
        emscripten::function("createUvMesh", &createUvMesh);
        emscripten::function("addMesh", &addMesh);
        emscripten::function("addUvMesh", &addUvMesh);
        emscripten::function("generateAtlas", &generateAtlas);
        emscripten::function("getMeshData", &getMeshData);
        emscripten::function("destroyAtlas", &destroyAtlas);
        emscripten::function("destroyMeshData", &destroyMeshData);
        emscripten::function("defaultChartOptions", &defaultChartOptions);
        emscripten::function("defaultPackOptions", &defaultPackOptions);
        emscripten::function("doLeakCheck", &__lsan_do_recoverable_leak_check);
}
