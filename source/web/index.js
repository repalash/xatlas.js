import {expose} from "comlink";
import {Api} from "./api.mjs"
import createXAtlasModule from "./build/xatlas.js"

expose(Api(createXAtlasModule));
