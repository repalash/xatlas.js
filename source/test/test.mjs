import { Worker } from 'worker_threads';
import nodeEndpoint from 'comlink/dist/umd/node-adapter.js';
import url from "url"
import * as path from "node:path"
import {wrap, proxy} from "comlink"

const __filename = url.fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function start(){
    const wasmFilePath = path.join(__dirname, "../../dist/node/xatlas.wasm");
    const worker = new Worker(`${__dirname}/../../dist/node/worker.mjs`);
    const api = await new (wrap(nodeEndpoint(worker)))(
        proxy(async ()=>{
            // onLoad();
            console.log("onLoad");
            console.log("api", api);
            await api.createAtlas();
            const dummyMesh = {
                indices: new Uint16Array([0, 1, 2]),
                vertices: new Float32Array([0, 0, 0, 1, 0, 0, 0, 1, 0]),
            }
            await api.addMesh(dummyMesh.indices, dummyMesh.vertices);
            const atlas = await api.generateAtlas({}, {});
            console.log("atlas", atlas);
            await api.destroyAtlas(atlas);
            await api.exit();
        }),
        proxy((path, dir) => {
            return (path === "xatlas.wasm" ? wasmFilePath : path + dir)
        }),
        proxy(()=>{
            console.log("onProgress");
        })
    );
}

start();
