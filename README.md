## xatlas.js 

xatlas.js is a wrapper for xatlas for js. It uses `emcc` to compile WASM from C++ codebase and can be used as a simple js module or as a webworker with [comlink](https://github.com/GoogleChromeLabs/comlink)

[xatlas](https://github.com/jpcy/xatlas) is a small C++11 library with no external dependencies that generates unique texture coordinates suitable for baking lightmaps or texture painting.
It is an independent fork of [thekla_atlas](https://github.com/Thekla/thekla_atlas), used by [The Witness](https://en.wikipedia.org/wiki/The_Witness_(2016_video_game)).

## How to use

### three.js
Check out [xatlas-three](https://github.com/repalash/xatlas-three) package to directly use xatlasjs in three.js with web-workers.

### Usage for Web and JS

* Add library to your `package.json` or do `npm install xatlasjs`
```json
  "devDependencies": {
    "xatlasjs": "^0.1.0"
  }
```
* Import or require class `XAtlasAPI` (from `dist/xatlas_web.js`) in your codebase and use wrapper functions for xatlas. See comments in `source/web/index.js`.
* Copy the file `dist/xatlas_web.wasm`, eg for webpack, install [CopyPlugin](https://webpack.js.org/plugins/copy-webpack-plugin/) and add this to the config
```javascript
      new CopyPlugin({
        patterns: [
          { from: path.resolve('node_modules', 'xatlasjs','dist','xatlas_web.wasm'), to: path.resolve(BUILD_PATH, 'libs/') },
        ],
      })
```
* Need to `locateFile` parameter function to the `XAtlasAPI` constructor if the `wasm` file is renamed or is not available from the website root.  

### Building Web
* Install emscripten 2. (Tested with 2.0.7 on macOS).
* Run `npm install`
* Run `npm run build` - this should generate files in the `dist` folder. 

### Generate an atlas (JS API)

First import the API class `import {XAtlasAPI} from "xatlasjs"` and create an object.
```javascript
const xAtlas = new XAtlasAPI(()=>{
        console.log("on module loadede");
    }, (path, dir)=>{
        if (path === "xatlas_web.wasm") return "libs/" + path;
        return dir + path;
    }, (mode, progress)=>{
        console.log("on progress ", mode, progress);
    }
);
```
Use the object `xAtlas` as:
1. Create an empty atlas with `createAtlas`.
2. Add one or more meshes with `addMesh`.
3. Call `generateAtlas`. Meshes are segmented into charts, which are parameterized and packed into an atlas. The updated vertex and index buffers are returned along with the mesh object.
4. See `source/web/index.js` for complete API and example.
The returned buffers are of different size than the inputs.
Cleanup with `destroyAtlas`. This also does a leak check if enabled in `build-web.sh`. see line 40. 


### Use as webworker, in JS API. 
This should be preferable, does not hang the web browser tab.
Load the xatlas_web.js file as web worker. For webpack, add to config:
```javascript
    rules: [
        {
            test: /\.worker\.js/,
            use: {
                loader: "worker-loader",
                options: { fallback: true }
            }
        }
    ]
```
Use in js example:
```javascript
import { wrap, proxy } from "comlink";
import XAtlasWorker from "xatlasjs";
/**
 * @class XAtlasAPI
 */
const XAtlasAPI = wrap(new XAtlasWorker());
let xAtlas = null;

// use in function 
async () => {
    if(xAtlas == null){
        xAtlas = await new XAtlasAPI(
                    proxy(()=>console.log("loaded")), 
                    proxy((path, dir)=>(path === "xatlas_web.wasm" ? "http://localhost:8000/libs/"+path:null)),
                    proxy((mode, progress)=> console.log("on progress ", mode, progress))
        );
    }
    while (!(await xAtlas.loaded)){
        await new Promise(r => setTimeout(r, 500)); // wait for load
    }
    await xAtlas.createAtlas();
    // Add mesh
    await xAtlas.addMesh(arguments);
    let meshes = await xAtlas.generateAtlas(chartOptions, packOptions, true);
    // Process meshes
    await xAtlas.destroyAtlas();
}
```

Full Readme of `xatlas` at its [main repository](https://github.com/jpcy/xatlas/blob/master/README.md).  

## Technical information / related publications

[Ignacio Castaño's blog post on thekla_atlas](http://www.ludicon.com/castano/blog/articles/lightmap-parameterization/)

P. Sander, J. Snyder, S. Gortler, and H. Hoppe. [Texture Mapping Progressive Meshes](http://hhoppe.com/proj/tmpm/)

K. Hormann, B. Lévy, and A. Sheffer. [Mesh Parameterization: Theory and Practice](http://alice.loria.fr/publications/papers/2007/SigCourseParam/param-course.pdf)

P. Sander, Z. Wood, S. Gortler, J. Snyder, and H. Hoppe. [Multi-Chart Geometry Images](http://hhoppe.com/proj/mcgim/)

D. Julius, V. Kraevoy, and A. Sheffer. [D-Charts: Quasi-Developable Mesh Segmentation](https://www.cs.ubc.ca/~vlady/dcharts/EG05.pdf)

B. Lévy, S. Petitjean, N. Ray, and J. Maillot. [Least Squares Conformal Maps for Automatic Texture Atlas Generation](https://members.loria.fr/Bruno.Levy/papers/LSCM_SIGGRAPH_2002.pdf)

O. Sorkine, D. Cohen-Or, R. Goldenthal, and D. Lischinski. [Bounded-distortion Piecewise Mesh Parameterization](https://igl.ethz.ch/projects/parameterization/BDPMP/index.php)

Y. O’Donnell. [Precomputed Global Illumination in Frostbite](https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf)

## Used by

[xatlas-three](https://github.com/repalash/xatlas-three)


## Related projects

[aobaker](https://github.com/prideout/aobaker) - Ambient occlusion baking. Uses [thekla_atlas](https://github.com/Thekla/thekla_atlas).

[Lightmapper](https://github.com/ands/lightmapper) - Hemicube based lightmap baking. The example model texture coordinates were generated by [thekla_atlas](https://github.com/Thekla/thekla_atlas).

[Microsoft's UVAtlas](https://github.com/Microsoft/UVAtlas) - isochart texture atlasing.

[Ministry of Flat](http://www.quelsolaar.com/ministry_of_flat/) - Commercial automated UV unwrapper.

[seamoptimizer](https://github.com/ands/seamoptimizer) - A C/C++ single-file library that minimizes the hard transition errors of disjoint edges in lightmaps.

[simpleuv](https://github.com/huxingyi/simpleuv/) - Automatic UV Unwrapping Library for Dust3D.
