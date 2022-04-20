const configLoader = require('./config-loader')
const { build } = require('..')

async function main (args) {
  const configFile = args[0]
  const config = await configLoader(configFile)
  await build(config)
}

main(process.argv.slice(2)).catch(err => {
  console.error(err)
  process.exit(1)
})
