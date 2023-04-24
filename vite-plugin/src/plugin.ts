import type { Plugin } from "vite"
import { readFile } from "fs/promises"

export default function plugin(): Plugin {
  return {
    name: "calory",
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        const match = /^\/local\-file(\/[^?]*)/.exec(req.url!)
        if (match) {
          const path = decodeURIComponent(match[1])
          readFile(path).then(buffer => {
            res.write(buffer)
            res.end()
          })
        }
        else {
          next()
        }
      })
    }
  }
}
