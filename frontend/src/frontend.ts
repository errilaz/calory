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

  /** Add a message listener. */
  export function addMessageListener(listener: MessageListener) {
    listeners.push(listener)
  }

  /** Remove a message listener. */
  export function removeMessageListener(listener: MessageListener) {
    const index = listeners.indexOf(listener)
    if (index > -1) listeners.splice(index, 1)
  }

  /** Send a message to the host. */
  export function send(message: any) {
    window.postMessage({ __envelope_to: "host", message })
  }

  /** Add a host message listener and return a function which cancels the subscription. */
  export function receive(listener: MessageListener) {
    addMessageListener(listener)
    return () => { removeMessageListener(listener) }
  }

  /** Intercepts function calls and sends a message in the form `{ method: string, parameters: any[] }` */
  export function proxy<Service>() {
    return new Proxy({}, {
      get(obj, method) {
        return (...parameters: any[]) => {
          send({ method, parameters })
        }
      }
    }) as Service
  }

  /** Receives messages in the form `{ method: string, parameters: any[] } and calls functions on the passed object.` */
  export function listen<Service>(proxy: Service) {
    return receive(message => {
      (proxy as any)[message.method](...message.parameters)
    })
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