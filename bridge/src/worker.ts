const manifest = chrome.runtime.getManifest()
const hostId = manifest.name.replace(/_bridge$/, "_host")

console.log(`connecting to host: ${hostId}`)

let host = chrome.runtime.connectNative(hostId)
let app: chrome.runtime.Port | null = null

host.onMessage.addListener(onMessageFromNative)
host.onDisconnect.addListener(onDisconnectNative)
chrome.runtime.onConnect.addListener(onConnectApp)

function onMessageFromNative(message: any) {
  app?.postMessage(message)
}

function onMessageFromApp(message: any) {
  host.postMessage(message)
}

function onConnectApp(port: chrome.runtime.Port) {
  app = port
  app.onMessage.addListener(onMessageFromApp)
}

function onDisconnectNative() {
  host.onDisconnect.removeListener(onDisconnectNative)
  host.onMessage.removeListener(onMessageFromNative)

  setTimeout(() => {
    host = chrome.runtime.connectNative(hostId)
    host.onMessage.addListener(onMessageFromNative)
    host.onDisconnect.addListener(onDisconnectNative)
  }, 250)
}
