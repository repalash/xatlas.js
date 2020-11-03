import createXAtlasModule from "./build/xatlas_web.js"

export class XAtlasAPI{

    /**
     * @param onLoad {Function}
     * @param locateFile {Function} - should return path for xatlas_web.wasm, default is root of domain
     */
    constructor(onLoad, locateFile) {
        this.xatlas = null;
        this.loaded = false;
        this.onLoad = onLoad ? [onLoad] : [];
        this.atlasCreated = false;
        /**
         * @type {{meshId: number, vertices: Float32Array, normals: Float32Array|null, coords: Float32Array|null, meshObj: any}[]}
         */
        this.meshes = [];
        let params = {}
        if (locateFile) params = {locateFile};
        createXAtlasModule(params).then(m=>{this.moduleLoaded(m)})
    }

    moduleLoaded(mod){
        this.xatlas = mod;
        this.loaded = true;
        for(let onLoad of this.onLoad) onLoad(mod);
    }

    createAtlas(){
        this.xatlas.createAtlas();
        this.atlasCreated = true;
    }

    /**
     *
     * @param indexes {Uint16Array}
     * @param vertices {Float32Array}
     * @param normals {Float32Array}
     * @param coords {Float32Array}
     * @param meshObj {any}
     * @param useNormals {boolean}
     * @param useCoords {boolean}
     * @param scale {number|[number, number, number]}
     * @return {null | {indexes: (Float32Array | null), vertices: Float32Array, normals: (Float32Array | null), meshId: number, coords: (Float32Array | null), meshObj: any}}
     */
    addMesh(indexes, vertices, normals=null, coords=null, meshObj=undefined, useNormals = false, useCoords = false, scale =1){
        if(!this.loaded || !this.atlasCreated) throw "Create atlas first";
        let meshDesc = this.xatlas.createMesh(vertices.length/3, indexes.length, normals != null && useNormals, coords != null && useCoords);
        this.xatlas.HEAPU16.set(indexes, meshDesc.indexOffset/2);

        let vs = new Float32Array([...vertices]);
        if(scale!==1) {
            if(typeof scale === "number") scale = [scale, scale, scale]
            for (let i = 0, l = vs.length; i < l; i+=3) {
                vs[i] *= scale[0];
                vs[i+1] *= scale[1];
                vs[i+2] *= scale[2];
            }
        }

        this.xatlas.HEAPF32.set(vs, meshDesc.positionOffset/4);
        if(normals != null && useNormals) this.xatlas.HEAPF32.set(normals, meshDesc.normalOffset/4);
        if(coords != null && useCoords) this.xatlas.HEAPF32.set(coords, meshDesc.uvOffset/4);
        let addMeshRes = this.xatlas.addMesh();
        // this.xatlas._free(meshDesc.indexOffset); // should be done on c++ side
        // this.xatlas._free(meshDesc.positionOffset);
        if(addMeshRes !== 0) {
            console.log("Error adding mesh: ", addMeshRes);
            return null;
        }
        let ret = {
            meshId: meshDesc.meshId,
            meshObj: meshObj,
            vertices: vertices,
            normals: normals || null,
            indexes: normals || null,
            coords: coords || null,
        };
        this.meshes.push(ret);
        return ret;
    }

    /**
     * @param vertexCount
     * @param indexCount
     * @param normals
     * @param coords
     * @return {{meshId: number, indexOffset: number, positionOffset: number, normalOffset: number, uvOffset: number, meshObj: any}}
     */
    createMesh(vertexCount, indexCount, normals, coords){
        return this.xatlas.createMesh(vertexCount, indexCount, normals, coords);
    }

    // createUvMesh(vertexCount, indexCount){
    //     return this.xatlas.createUvMesh(vertexCount, indexCount);
    // }

    /**
     * Result in coords1, input coords in coords
     * @param chartOptions {{maxIterations: number, straightnessWeight: number, textureSeamWeight: number, maxChartArea: number, normalDeviationWeight: number, roundnessWeight: number, maxCost: number, maxBoundaryLength: number, normalSeamWeight: number}}
     * @param packOptions {{maxChartSize: number, padding: number, bilinear: boolean, createImage: boolean, blockAlign: boolean, resolution: number, bruteForce: boolean, texelsPerUnit: number}}
     * @param returnMeshes {boolean} - default = true
     * @return {{vertex: {vertices: Float32Array, coords1: Float32Array, normals?: Float32Array, coords?: Float32Array}, index: Uint16Array, mesh: any}[]}
     */
    generateAtlas(chartOptions, packOptions, returnMeshes = true){
        if(!this.loaded || !this.atlasCreated) throw "Create atlas first";
        if(this.meshes.length < 1) throw "Add meshes first";
        chartOptions = { ...this.xatlas.defaultChartOptions(), ...chartOptions};
        packOptions = { ...this.xatlas.defaultPackOptions(), ...packOptions };
        this.xatlas.generateAtlas(chartOptions, packOptions);
        if(!returnMeshes) return [];
        let returnVal = [];
        for (let {meshId, meshObj, vertices, normals, coords} of this.meshes){
            let ret = this.getMeshData(meshId);
            let index = new Uint16Array(this.xatlas.HEAPU32.subarray(ret.indexOffset/4, ret.indexOffset/4+ret.newIndexCount));
            let oldIndexes = new Uint16Array(this.xatlas.HEAPU32.subarray(ret.originalIndexOffset/4, ret.originalIndexOffset/4+ret.newVertexCount));
            let xcoords = new Float32Array(this.xatlas.HEAPF32.subarray(ret.uvOffset/4, ret.uvOffset/4+ret.newVertexCount*2));
            this.xatlas.destroyMeshData(ret);

            let vertex = {};
            vertex.vertices = new Float32Array(ret.newVertexCount * 3);
            vertex.coords1 = xcoords;
            if(normals)
                vertex.normals = new Float32Array(ret.newVertexCount * 3);
            if(coords)
                vertex.coords = new Float32Array(ret.newVertexCount * 2);
            else vertex.coords = vertex.coords1;

            for(let i =0, l=ret.newVertexCount; i<l; i++){
                let oldIndex = oldIndexes[i];
                vertex.vertices[3*i + 0] = vertices[3*oldIndex + 0];
                vertex.vertices[3*i + 1] = vertices[3*oldIndex + 1];
                vertex.vertices[3*i + 2] = vertices[3*oldIndex + 2];
                if(vertex.normals&&normals){
                    vertex.normals[3*i + 0] = normals[3*oldIndex + 0];
                    vertex.normals[3*i + 1] = normals[3*oldIndex + 1];
                    vertex.normals[3*i + 2] = normals[3*oldIndex + 2];
                }
                if(vertex.coords&&coords){
                    vertex.coords[2*i + 0] = coords[2*oldIndex + 0];
                    vertex.coords[2*i + 1] = coords[2*oldIndex + 1];
                }
            }
            returnVal.push({index: index, vertex: vertex, mesh: meshObj})
        }
        return returnVal;
    }

    /**
     * @return {{maxIterations: number, straightnessWeight: number, textureSeamWeight: number, maxChartArea: number, normalDeviationWeight: number, roundnessWeight: number, maxCost: number, maxBoundaryLength: number, normalSeamWeight: number}}
     */
    defaultChartOptions() {
        return {
            "maxChartArea": 0,
            "maxBoundaryLength": 0,
            "normalDeviationWeight": 2,
            "roundnessWeight": 0.009999999776482582,
            "straightnessWeight": 6,
            "normalSeamWeight": 4,
            "textureSeamWeight": 0.5,
            "maxCost": 2,
            "maxIterations": 1
        };
    }

    /**
     * @return {{maxChartSize: number, padding: number, bilinear: boolean, createImage: boolean, blockAlign: boolean, resolution: number, bruteForce: boolean, texelsPerUnit: number}}
     */
    defaultPackOptions() {
        return {
            "bilinear": true,
            "blockAlign": false,
            "bruteForce": false,
            "createImage": false,
            "maxChartSize": 0,
            "padding": 0,
            "texelsPerUnit": 0,
            "resolution": 0
        };
    }

    /**
     * @param meshId
     * @return {{newVertexCount: number, newIndexCount: number, indexOffset: number, originalIndexOffset: number, uvOffset: number}}
     */
    getMeshData(meshId){
        return this.xatlas.getMeshData(meshId);
    }

    /**
     * @param data {{newVertexCount: number, newIndexCount: number, indexOffset: number, originalIndexOffset: number, uvOffset: number}}
     * @return {*}
     */
    destroyMeshData(data){
        this.xatlas.destroyMeshData(data);
    }

    destroyAtlas(){
        this.atlasCreated = false;
        this.xatlas.destroyAtlas();
        this.xatlas.doLeakCheck();
    }

    /**
     * @param fn {Function}
     */
    addOnLoad (fn) {
        this.onLoad.push(fn)
    }
}
