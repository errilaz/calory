const worker = chrome.runtime.connect({ name: "calory" })

interface WindowEnvelope {
  __envelope_to: "host" | "app"
  message: any
}

worker.onMessage.addListener(message => {
  window.postMessage({ __envelope_to: "app", message })
})

window.addEventListener("message", message => {
  if (message.data?.__envelope_to === "host") {
    worker.postMessage(message.data.message)
  }
})
