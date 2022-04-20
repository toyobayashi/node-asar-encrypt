const path = require('path')
const { defineConfiguration } = require('..')

module.exports = defineConfiguration({
  appDir: path.join(__dirname, '../test'),
  outDir: path.join(__dirname, '../out'),
  noAsar: false,
  splitNodeModules: true,
  requireFromMain: './app'
})
