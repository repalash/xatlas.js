import {expose} from "comlink";
import {parentPort} from "node:worker_threads";
import nodeEndpoint from 'comlink/dist/umd/node-adapter.js';
import {Api} from "./api.mjs"
import createXAtlasModule from "./xatlas.js"

if (!parentPort) throw new Error('InvalidWorker');

const api = Api(createXAtlasModule)
api.prototype.exit = ()=>process.exit();

expose(api, nodeEndpoint(parentPort));
