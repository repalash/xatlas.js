export const Api = (createXAtlasModule)=>{
    let _onLoad = ()=>{} //  we cannot put it in the object, otherwise we cannot access it from the outside
    return class XatlasApi {

        /**
         * @param onLoad {Function}
         * @param locateFile {Function} - should return path for xatlas.wasm, default is root of domain
         * @param onAtlasProgress {Function} - called on progress update with mode {ProgressCategory} and counter
         */
        constructor(onLoad, locateFile, onAtlasProgress) {
            this.xatlas = null;
            this.loaded = false;
            _onLoad = onLoad || (()=>{});
            this.atlasCreated = false;
            /**
             * @type {{meshId: number, vertices: Float32Array, normals: Float32Array|null, coords: Float32Array|null, meshObj: any}[]}
             */
            this.meshes = [];
            let params = {};
            if (onAtlasProgress) params = {...params, onAtlasProgress};
            const ctor = (loc)=>{
                params = {...params, locateFile: ((path, dir)=> ( (loc && path === "xatlas.wasm") ? loc : dir+path) ) };
                createXAtlasModule(params).then(m=>{this.moduleLoaded(m)});
            }
            if (locateFile) {
                const pp = locateFile("xatlas.wasm", "") // separately because it can return a promise
                if (pp&&pp.then) pp.then(ctor);
                else ctor(pp);
            }else ctor()
        }

        moduleLoaded(mod){
            this.xatlas = mod;
            this.loaded = true;
            if(_onLoad) _onLoad();
        }

        createAtlas(){
            this.xatlas.createAtlas();
            this.meshes = [];
            this.atlasCreated = true;
        }

        /**
         *
         * @param indexes {Uint16Array}
         * @param vertices {Float32Array}
         * @param normals {Float32Array}
         * @param coords {Float32Array}
         * @param meshObj {any} - identifier for the mesh (uuid)
         * @param useNormals {boolean}
         * @param useCoords {boolean}
         * @param scale {number|[number, number, number]}
         * @return {null | {indexes: (Float32Array | null), vertices: Float32Array, normals: (Float32Array | null), meshId: number, coords: (Float32Array | null), meshObj: any}}
         */
        addMesh(indexes, vertices, normals=null, coords=null, meshObj=undefined, useNormals = false, useCoords = false, scale =1){
            if(!this.loaded || !this.atlasCreated) throw "Create atlas first";
            const meshDesc = this.xatlas.createMesh(vertices.length / 3, indexes.length, normals != null && useNormals, coords != null && useCoords)
            this.xatlas.HEAPU16.set(indexes, meshDesc.indexOffset/2);

            const vs = new Float32Array([...vertices])
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
            const addMeshRes = this.xatlas.addMesh()
            // this.xatlas._free(meshDesc.indexOffset); // should be done on c++ side
            // this.xatlas._free(meshDesc.positionOffset);
            if(addMeshRes !== 0) {
                console.log("Error adding mesh: ", addMeshRes);
                return null;
            }
            const ret = {
                meshId: meshDesc.meshId,
                meshObj: meshObj,
                vertices: vertices,
                normals: normals || null,
                indexes: indexes || null,
                coords: coords || null,
            }
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
         */
        generateAtlas(chartOptions, packOptions, returnMeshes = true){
            if(!this.loaded || !this.atlasCreated) throw "Create atlas first";
            if(this.meshes.length < 1) throw "Add meshes first";
            chartOptions = { ...this.defaultChartOptions(), ...chartOptions};
            packOptions = { ...this.defaultPackOptions(), ...packOptions };
            this.xatlas.generateAtlas(chartOptions, packOptions);
            if(!returnMeshes) return [];
            return this.getAtlas()
        }

        getAtlas() {
            const returnVal = []
            if(!this.loaded || !this.atlasCreated) throw "Create atlas first";
            const _atlas = this.xatlas.getAtlas()
            const atlas = {
                width: _atlas.width,
                height: _atlas.height,
                atlasCount: _atlas.atlasCount,
                meshCount: _atlas.meshCount,
                // chartCount: atlas.chartCount,
                // utilization: atlas.utilization,
                // meshes: this.getMeshData
                texelsPerUnit: _atlas.texelsPerUnit,
                // image:
            }
            for (const {meshId, meshObj, vertices, normals, coords} of this.meshes) {
                const ret = this.getMeshData(meshId)
                const vCount = ret.newVertexCount;
                const index = new Uint16Array(this.xatlas.HEAPU32.subarray(ret.indexOffset / 4, ret.indexOffset / 4 + ret.newIndexCount))
                const oldIndexes = new Uint16Array(this.xatlas.HEAPU32.subarray(ret.originalIndexOffset / 4, ret.originalIndexOffset / 4 + vCount))
                // const atlasIndexes = atlas.atlasCount > 1 ? new Uint16Array(this.xatlas.HEAPU32.subarray(ret.atlasIndexOffset / 4, ret.atlasIndexOffset / 4 + ret.newIndexCount)) : null;
                const xcoords = new Float32Array(this.xatlas.HEAPF32.subarray(ret.uvOffset / 4, ret.uvOffset / 4 + vCount * 2))

                const subMeshes = [];
                for (let i = 0, n=ret.subMeshes.size(); i < n; i++) {
                    subMeshes.push(ret.subMeshes.get(i));
                }
                this.xatlas.destroyMeshData(ret);
                ret.subMeshes.delete();

                const vertex = {}
                vertex.vertices = new Float32Array(vCount * 3);
                vertex.coords1 = xcoords;
                if (normals)
                    vertex.normals = new Float32Array(vCount * 3);
                if (coords)
                    vertex.coords = new Float32Array(vCount * 2);
                else vertex.coords = vertex.coords1;

                for (let i = 0, l = vCount; i < l; i++) {
                    const oldIndex = oldIndexes[i]
                    vertex.vertices[3 * i + 0] = vertices[3 * oldIndex + 0];
                    vertex.vertices[3 * i + 1] = vertices[3 * oldIndex + 1];
                    vertex.vertices[3 * i + 2] = vertices[3 * oldIndex + 2];
                    if (vertex.normals && normals) {
                        vertex.normals[3 * i + 0] = normals[3 * oldIndex + 0];
                        vertex.normals[3 * i + 1] = normals[3 * oldIndex + 1];
                        vertex.normals[3 * i + 2] = normals[3 * oldIndex + 2];
                    }
                    if (vertex.coords && coords) {
                        vertex.coords[2 * i + 0] = coords[2 * oldIndex + 0];
                        vertex.coords[2 * i + 1] = coords[2 * oldIndex + 1];
                    }
                }
                returnVal.push({
                    index: index,
                    vertex: vertex,
                    mesh: meshObj,
                    vertexCount: vCount,
                    oldIndexes: oldIndexes,
                    subMeshes: subMeshes
                });
            }
            return {...atlas, meshes: returnVal};
            // return returnVal;
        }

        defaultChartOptions() {
            return {
                fixWinding: false,
                maxBoundaryLength: 0,
                maxChartArea: 0,
                maxCost: 2,
                maxIterations: 1,
                normalDeviationWeight: 2,
                normalSeamWeight: 4,
                roundnessWeight: 0.009999999776482582,
                straightnessWeight: 6,
                textureSeamWeight: 0.5,
                useInputMeshUvs: false,
            };
        }

        defaultPackOptions() {
            return {
                bilinear: true,
                blockAlign: false,
                bruteForce: false,
                createImage: false,
                maxChartSize: 0,
                padding: 0,
                resolution: 0,
                rotateCharts: true,
                rotateChartsToAxis: true,
                texelsPerUnit: 0
            };
        }

        setProgressLogging(flag){
            this.xatlas.setProgressLogging(flag);
        }

        /**
         * @param meshId
         * @return {{newVertexCount: number, newIndexCount: number, indexOffset: number, originalIndexOffset: number, uvOffset: number, atlasIndexOffset: number}}
         */
        getMeshData(meshId){
            return this.xatlas.getMeshData(meshId);
        }

        /**
         * @param data {{newVertexCount: number, newIndexCount: number, indexOffset: number, originalIndexOffset: number, uvOffset: number, atlasIndexOffset: number}}
         * @return {*}
         */
        destroyMeshData(data){
            this.xatlas.destroyMeshData(data);
        }

        destroyAtlas(){
            this.atlasCreated = false;
            this.xatlas.destroyAtlas();
            this.meshes = [];
            this.xatlas.doLeakCheck();
        }

    }
};
