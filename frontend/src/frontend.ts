// backend.addEventListener
// backend.send({ ... })
// readFile()

export module Backend {
  export type MessageListener = (message: any) => any
  const listeners: MessageListener[] = []

  window.addEventListener("message", event => {
    if (event.data?.__envelope_to === "app") {
      for (const listener of listeners) {
        listener(event.data.message)
      }
    }
  })

  export function addMessageListener(listener: MessageListener) {
    listeners.push(listener)
  }

  export function removeMessageListener(listener: MessageListener) {
    const index = listeners.indexOf(listener)
    if (index > -1) listeners.splice(index, 1)
  }

  export function send(message: any) {
    window.postMessage({ __envelope_to: "host", message })
  }
}

export function readFileAsString(path: string) {
  return requestString(localUrl(path))
}

export function localUrl(path: string) {
  if (location.protocol === "http:") {
    return `/local-file${path}`
  }
  else {
    return `file://${path}`
  }
}

function requestString(url: string): Promise<string> {
  const req = new XMLHttpRequest()
  return new Promise((resolve, reject) => {
    req.addEventListener("load", function () {
      resolve(this.responseText)
    })
    req.addEventListener("error", function () {
      reject(new Error(this.statusText))
    })
    req.open("GET", url)
    req.send()
  })
}