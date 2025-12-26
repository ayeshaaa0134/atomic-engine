const path = require('path');
const webpack = require('webpack');

/** @typedef {import('webpack').Configuration} WebpackConfig */

/** @type {WebpackConfig} */
const extensionConfig = {
    mode: 'none',
    target: 'node',
    entry: {
        extension: './src/extension.ts'
    },
    output: {
        filename: '[name].js',
        path: path.join(__dirname, './dist'),
        libraryTarget: 'commonjs',
        devtoolModuleFilenameTemplate: '../../[resource-path]'
    },
    resolve: {
        mainFields: ['module', 'main'],
        extensions: ['.ts', '.js']
    },
    module: {
        rules: [
            {
                test: /\.ts$/,
                exclude: /node_modules/,
                use: [
                    {
                        loader: 'ts-loader'
                    }
                ]
            }
        ]
    },
    externals: {
        'vscode': 'commonjs vscode'
    },
    performance: {
        hints: false
    },
    devtool: 'nosources-source-map'
};

/** @type {WebpackConfig} */
const webviewConfig = {
    mode: 'development', // Changed to dev for better debugging
    target: 'web',
    entry: {
        webview: './src/webview/main.tsx'
    },
    output: {
        filename: '[name].js',
        path: path.join(__dirname, './dist'),
        // Removed libraryTarget 'umd'. Default or 'iife' is better for direct script inclusion.
    },
    resolve: {
        mainFields: ['module', 'main'],
        extensions: ['.ts', '.tsx', '.js']
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                exclude: /node_modules/,
                use: [
                    {
                        loader: 'ts-loader',
                        options: {
                            transpileOnly: true
                        }
                    }
                ]
            },
            {
                test: /\.css$/,
                use: ['style-loader', 'css-loader']
            }
        ]
    },
    performance: {
        hints: false
    },
    devtool: 'inline-source-map'
};

module.exports = [extensionConfig, webviewConfig];
