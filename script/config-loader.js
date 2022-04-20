const fs = require('fs-extra')
const path = require('path')

function getDefaultExport (exports) {
  if (exports.__esModule) {
    return exports['default']
  }
  return exports
}

/**
 * @param {string} configFile 
 * @returns {Promise<import('..').IConfiguration>}
 */
async function load (configFile) {
  if (!configFile) {
    throw new Error('missing input config file.')
  }
  
  const configFileAbsolutePath = path.resolve(configFile)
  if (!fs.existsSync(configFileAbsolutePath)) {
    throw new Error('invalid config file.')
  }
  
  const configExports = getDefaultExport(require(configFileAbsolutePath))
  const config = typeof configExports === 'function' ? await Promise.resolve(configExports()) : configExports

  return config
}

module.exports = load
