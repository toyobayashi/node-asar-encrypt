export interface IConfiguration {
  /**
   * absolute path to node project
   */
  appDir: string

  /**
   * absolute path to output
   */
  outDir: string

  /**
   * do not use asar
   * @defaultValue `false`
   */
  noAsar?: boolean

  /** 
   * split node_modules to node_modules.asar
   * if noAsar === true this option will be ignored
   * @defaultValue `false`
   */
  splitNodeModules?: boolean

  /**
   * application entry from native addon
   * @defaultValue `'./app'`
   */
  requireFromMain?: string
}

export type Configuration = IConfiguration | (() => IConfiguration | Promise<IConfiguration>)

export function defineConfiguration <T extends Configuration> (c: T): T

export function build (config: IConfiguration): Promise<void>
