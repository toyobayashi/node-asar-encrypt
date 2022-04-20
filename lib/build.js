const fs = require('fs-extra')
const path = require('path')
const getPath = require('./path.js')
const crypto = require('crypto')
const { spawn } = require('./spawn')
const { createPackageWithOptions, extractAll } = require('asar')
const js2c = require('./js2c')

function writeKey (target) {
  fs.writeFileSync(target, Array.prototype.map.call(crypto.randomBytes(32), (v => ('0x' + ('0' + v.toString(16)).slice(-2)))).toString())
}

async function build (config) {
  if (!config) {
    throw new TypeError('Missing input configuration')
  }

  writeKey(getPath('src/key.txt'))
  js2c(config)

  const outDir = path.resolve(config.outDir)
  fs.removeSync(outDir)
  fs.mkdirSync(outDir, { recursive: true })

  const requireFromMain = config.requireFromMain || './app'
  await spawn(getPath(`node_modules/.bin/node-gyp${process.platform === 'win32' ? '.cmd' : ''}`), [
    'rebuild',
    `--require_from_main=${requireFromMain}`,
    ...(config.noAsar ? ['--no_asar'] : [])
  ], getPath('src'))
  fs.copySync(getPath('src/build/Release/main.node'), path.join(outDir, 'main.node'))

  const appDir = path.resolve(config.appDir)
  const key = Buffer.from(fs.readFileSync(getPath('src/key.txt'), 'utf8').trim().split(',').map(v => Number(v.trim())))
  const appAsar = path.join(outDir, 'app.asar')
  await createPackageWithOptions(appDir, appAsar, {
    unpack: '*.node',
    ...((!config.noAsar && config.splitNodeModules) ? { pattern: '/{!(node_modules)*,!(node_modules)/**/*}' } : {}),
    transform (filename) {
      if (path.extname(filename) === '.js') {
        const iv = crypto.randomBytes(16)
        var append = false
        var cipher = crypto.createCipheriv('aes-256-cbc', key, iv)
        cipher.setAutoPadding(true)
        cipher.setEncoding('base64')
  
        const _p = cipher.push
        cipher.push = function (chunk, enc) {
          if (!append && chunk != null) {
            append = true
            return _p.call(this, Buffer.concat([iv, chunk]), enc)
          } else {
            return _p.call(this, chunk, enc)
          }
        }
        return cipher
      }
    }
  })

  if (!config.noAsar && config.splitNodeModules) {
    const nodeModulesDir = path.join(appDir, 'node_modules')
    if (fs.existsSync(nodeModulesDir) && fs.statSync(nodeModulesDir).isDirectory()) {
      await createPackageWithOptions(nodeModulesDir, path.join(outDir, 'node_modules.asar'), {
        unpack: '*.node'
      })
    }
  }

  if (config.noAsar) {
    extractAll(appAsar, path.join(outDir, 'app'))
    fs.removeSync(appAsar)
    fs.removeSync(appAsar + '.unpacked')
  }
}

module.exports = build
