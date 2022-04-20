const fs = require('fs')
const path = require('path')
const getPath = require('./path.js')
let terser
try {
  terser = require('terser')
} catch (_) {
  terser = {
    minify (code) {
      return { code }
    }
  }
}

function wrap (code) {
  return `(${code});`
}

function wrapCJS (code) {
  return `(function (require) {
    const module = {};
    const exports = module.exports = {};
    (function (require, exports, module) {
      ${code}
    })(require, exports, module);
  });`
}

function str2buf (str) {
  const zero = Buffer.alloc(1)
  zero[0] = 0
  return Buffer.concat([Buffer.from(str), zero])
}

function buf2pchar(buf, varname) {
  return `const char ${varname}[]={${Array.prototype.join.call(buf, ',')}};`
}

module.exports = function (config) {
  const options = {}

  const result1 = terser.minify(fs.readFileSync(getPath('src/find.js'), 'utf8'), options);
  const result2 = terser.minify(fs.readFileSync(getPath('src/require.js'), 'utf8'), options);
  
  
  const scriptFind = buf2pchar(str2buf(wrap(result1.code)), 'scriptFind')
  const scriptRequire = buf2pchar(str2buf(wrap(result2.code)), 'scriptRequire')
  let scriptAsarNode = ''
  if (!config.noAsar) {
    const asarNodeMinifiedCode = fs.readFileSync(path.join(path.dirname(require.resolve('asar-node')), 'dist/autorun.js'), 'utf8')
    scriptAsarNode = buf2pchar(str2buf(wrapCJS(asarNodeMinifiedCode)), 'scriptAsarNode')
  }

  fs.writeFileSync(getPath('src/script.h'), scriptFind + '\n' + scriptRequire + '\n' + scriptAsarNode + '\n', 'utf8')
}
