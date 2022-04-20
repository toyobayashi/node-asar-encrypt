# node-asar-encrypt

Similar solution to [toyobayashi/electron-asar-encrypt-demo](https://github.com/toyobayashi/electron-asar-encrypt-demo) (Chinese)

## Usage

```bash
git clone ...
cd ...
npm install
```

```
node ./script/build <your-config.js>
```

```js
// config file

const path = require('path')

module.exports = function () {
  // return Promise is ok
  return {
    // absolute path to node project
    appDir: path.join(__dirname, 'test'),
    // absolute path to output
    outDir: path.join(__dirname, 'out'),
    // do not use asar
    noAsar: false,
    // split node_modules to node_modules.asar
    // if noAsar === true this option will be ignored
    splitNodeModules: true,
    // application entry from native addon
    requireFromMain: './app'
  }
}

// config object is ok
// module.exports = { ... }
```

## Example

```bash
cd test
npm install
cd ..

npm run build
node out/main.node
```
