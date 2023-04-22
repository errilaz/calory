const worker = chrome.runtime.connect({ name: "calory" })

interface WindowEnvelope {
  __envelope_to: "host" | "app"
  message: any
}

worker.onMessage.addListener(message => {
  window.postMessage({ __envelope_to: "app", message })
})

window.addEventListener("message", event => {
  if (event.data?.__envelope_to === "host") {
    worker.postMessage(event.data.message)
  }
})
