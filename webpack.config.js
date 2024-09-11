const webpack = require("webpack");
const path = require("path");

const SRC_PATH = path.resolve(__dirname, 'source', 'web');
const NODE_MOD_PATH = path.resolve(__dirname, 'node_modules');
const BUILD_PATH = path.resolve(__dirname, 'dist');
const BUILD_FILE_NAME = 'xatlas';
const LIBRARY_NAME = 'XAtlas';
const LIBRARY_TARGET = 'self';
// const MODE = "development"
const MODE = "production"

const entry = {
};

entry[BUILD_FILE_NAME] = path.join(SRC_PATH, 'index.js');

module.exports = {
    mode: MODE,
    devtool: 'source-map',
    entry: entry,
    output: {
        filename: '[name].js',
        path: BUILD_PATH,
        library: {
            name: LIBRARY_NAME,
            type: LIBRARY_TARGET,
        },
        globalObject: 'this',
    },
    resolve: {
        modules: [SRC_PATH, NODE_MOD_PATH],
        fallback: {
            fs: false,
        }
    },
    module: {
        rules: [
            {
                test: /\.worker\.js/,
                use: {
                    loader: "worker-loader",
                    options: { fallback: true }
                },
                type: 'javascript/auto'
            },
            {
                test: /\.wasm$/,
                type: 'javascript/auto',
                loader: 'file-loader',
            }
        ]
    },
    experiments: {
        syncWebAssembly: true,
    }
};
