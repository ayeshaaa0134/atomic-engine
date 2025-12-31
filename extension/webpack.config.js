const path = require('path');

module.exports = {
    mode: 'production',
    target: 'node',
    entry: {
        extension: './src/extension.ts'
    },
    output: {
        path: path.resolve(__dirname, 'dist'),
        filename: '[name].js',
        libraryTarget: 'commonjs',
    },
    resolve: {
        extensions: ['.ts', '.js'],
    },
    module: {
        rules: [
            {
                test: /\.ts$/,
                use: [
                    {
                        loader: 'ts-loader',
                    },
                ],
                exclude: /node_modules/,
            },
        ],
    },
    externals: {
        vscode: 'commonjs vscode',
    },
};
