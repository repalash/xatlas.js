#include "xatlas_web.h"

xatlas::Atlas *atlas;
xatlas::MeshDecl *meshDecl;
xatlas::UvMeshDecl *uvMeshDecl;
uint32_t nextMeshId = 0;
bool printProgress = false;

static bool ProgressCallback(xatlas::ProgressCategory category, int progress, void *userData)
{
    if(printProgress) {
        printf("%s [", xatlas::StringForEnum(category));
        for (int i = 0; i < 10; i++)
            printf(progress / ((i + 1) * 10) ? "*" : " ");
        printf("] %d%%\n", progress);
    }
    onAtlasProgress((int)category, progress);
    return true;
}

void setProgressLogging(bool f){
    printProgress = f;
}

void createAtlas() {
    atlas = xatlas::Create();
    xatlas::SetProgressCallback(atlas, ProgressCallback, nullptr);
}

xatlas::Atlas getAtlas() {
    return *atlas;
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

void computeCharts(const xatlas::ChartOptions chartOptions = defaultChartOptions()){
    xatlas::ComputeCharts(atlas, chartOptions);
}
void packCharts(const xatlas::PackOptions packOptions=defaultPackOptions()){
    xatlas::PackCharts(atlas, packOptions);
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
//     int32_t *atlasIndexArray = new int32_t[mesh.vertexCount];
    float *uvArray = new float[mesh.vertexCount * 2];

    bool hasSubAtlas = atlas->atlasCount > 1;
    // stable_sort by atlasIndex
    if(hasSubAtlas) {
        if(mesh.indexArray)
            std::stable_sort(mesh.indexArray, mesh.indexArray + mesh.indexCount, [&](const uint32_t &a, const uint32_t &b) {
                return mesh.vertexArray[a].atlasIndex < mesh.vertexArray[b].atlasIndex;
            });
        else
            std::stable_sort(mesh.vertexArray, mesh.vertexArray + mesh.vertexCount, [](const xatlas::Vertex &a, const xatlas::Vertex &b) {
                return a.atlasIndex < b.atlasIndex;
            });
    }

    std::vector<SubAtlasMeshInfo> subMeshes;

    for (uint32_t i = 0; i < mesh.vertexCount; i++) {
        const xatlas::Vertex &vertex = mesh.vertexArray[i];
        originalIndexArray[i] = vertex.xref;
        // atlasIndexArray[i] = vertex.atlasIndex;
        uvArray[i * 2] = vertex.uv[0] / atlas->width;
        uvArray[i * 2 + 1] = vertex.uv[1] / atlas->height;
    }

    if(hasSubAtlas){
        uint32_t start = 0;
        uint32_t count = 0;
        uint32_t vertexCount = mesh.indexArray ? mesh.indexCount : mesh.vertexCount;
        // uint32_t atlasIndex = vertexCount > 0 ? mesh.vertexArray[0].atlasIndex : 0;
        uint32_t atlasIndex = vertexCount > 0 ? mesh.vertexArray[mesh.indexArray ? mesh.indexArray[0] : 0].atlasIndex : 0;
        for (uint32_t i = 0; i < vertexCount; i++) {
            count++;
            uint32_t currentIndex = mesh.vertexArray[mesh.indexArray ? mesh.indexArray[i] : i].atlasIndex;
            if (hasSubAtlas && (currentIndex != atlasIndex || i == mesh.vertexCount - 1)) {
                SubAtlasMeshInfo subMesh;
                subMesh.start = start;
                subMesh.count = count;
                subMesh.atlasIndex = atlasIndex;
                subMeshes.push_back(subMesh);
                start = i;
                count = 0;
                atlasIndex = currentIndex;
            }
        }
    }


    AtlasMeshBufferInfo atlasMeshBufferInfo;

    atlasMeshBufferInfo.newVertexCount = mesh.vertexCount;
    atlasMeshBufferInfo.newIndexCount = mesh.indexCount;
    atlasMeshBufferInfo.indexOffset = (uint32_t) mesh.indexArray;
    atlasMeshBufferInfo.originalIndexOffset = (uint32_t) originalIndexArray;
    atlasMeshBufferInfo.uvOffset = (uint32_t) uvArray;
    atlasMeshBufferInfo.subMeshes = subMeshes;
//     atlasMeshBufferInfo.atlasIndexOffset = (int32_t) atlasIndexArray;

    return atlasMeshBufferInfo;
}

void destroyMeshData(const AtlasMeshBufferInfo atlasMeshBufferInfo) {
    delete[] (uint32_t *) atlasMeshBufferInfo.originalIndexOffset;
    delete[] (float *) atlasMeshBufferInfo.uvOffset;
//     delete[] (int32_t *) atlasMeshBufferInfo.atlasIndexOffset;
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
        emscripten::value_object<xatlas::Atlas>("Atlas")
                .field("width", &xatlas::Atlas::width)
                .field("height", &xatlas::Atlas::height)
                .field("atlasCount", &xatlas::Atlas::atlasCount)
//                 .field("chartCount", &xatlas::Atlas::chartCount)
                .field("meshCount", &xatlas::Atlas::meshCount)
                .field("texelsPerUnit", &xatlas::Atlas::texelsPerUnit);

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

        emscripten::value_object<SubAtlasMeshInfo>("SubAtlasMeshInfo")
        .field("start", &SubAtlasMeshInfo::start)
        .field("count", &SubAtlasMeshInfo::count)
        .field("atlasIndex", &SubAtlasMeshInfo::atlasIndex);

        emscripten::value_object<AtlasMeshBufferInfo>("AtlasMeshBufferInfo")
        .field("newVertexCount", &AtlasMeshBufferInfo::newVertexCount)
        .field("newIndexCount", &AtlasMeshBufferInfo::newIndexCount)
        .field("indexOffset", &AtlasMeshBufferInfo::indexOffset)
        .field("originalIndexOffset", &AtlasMeshBufferInfo::originalIndexOffset)
//         .field("atlasIndexOffset", &AtlasMeshBufferInfo::atlasIndexOffset)
        .field("uvOffset", &AtlasMeshBufferInfo::uvOffset)
        .field("subMeshes", &AtlasMeshBufferInfo::subMeshes);

        emscripten::register_vector<SubAtlasMeshInfo>("SubAtlasMeshInfoVector");

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
        .field("useInputMeshUvs", &xatlas::ChartOptions::useInputMeshUvs)
        .field("fixWinding", &xatlas::ChartOptions::fixWinding)
        ;

        emscripten::value_object<xatlas::PackOptions>("PackOptions")
        .field("maxChartSize", &xatlas::PackOptions::maxChartSize)
        .field("padding", &xatlas::PackOptions::padding)
        .field("texelsPerUnit", &xatlas::PackOptions::texelsPerUnit)
        .field("resolution", &xatlas::PackOptions::resolution)
        .field("bilinear", &xatlas::PackOptions::bilinear)
        .field("blockAlign", &xatlas::PackOptions::blockAlign)
        .field("bruteForce", &xatlas::PackOptions::bruteForce)
        .field("createImage", &xatlas::PackOptions::createImage)
        .field("rotateChartsToAxis", &xatlas::PackOptions::rotateChartsToAxis)
        .field("rotateCharts", &xatlas::PackOptions::rotateCharts)
        ;

        emscripten::function("createAtlas", &createAtlas);
        emscripten::function("getAtlas", &getAtlas);
        emscripten::function("createMesh", &createMesh);
        emscripten::function("createUvMesh", &createUvMesh);
        emscripten::function("addMesh", &addMesh);
        emscripten::function("addUvMesh", &addUvMesh);
        emscripten::function("generateAtlas", &generateAtlas);
        emscripten::function("computeCharts", &computeCharts);
        emscripten::function("packCharts", &packCharts);
        emscripten::function("getMeshData", &getMeshData);
        emscripten::function("destroyAtlas", &destroyAtlas);
        emscripten::function("destroyMeshData", &destroyMeshData);
        emscripten::function("defaultChartOptions", &defaultChartOptions);
        emscripten::function("defaultPackOptions", &defaultPackOptions);
        emscripten::function("setProgressLogging", &setProgressLogging);
        emscripten::function("doLeakCheck", &__lsan_do_recoverable_leak_check);
}
