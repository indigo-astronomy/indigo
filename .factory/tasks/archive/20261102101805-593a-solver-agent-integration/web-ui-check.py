import base64, json, socket, sys, time

OUT = sys.argv[1]
BASE = "http://localhost:7700/"


class Marionette:
	def __init__(self, port=2828):
		for _ in range(60):
			try:
				self.sock = socket.create_connection(("127.0.0.1", port))
				break
			except OSError:
				time.sleep(0.5)
		self.buf = b""
		self.id = 0
		self.read()

	def read(self):
		while b":" not in self.buf:
			self.buf += self.sock.recv(65536)
		length, rest = self.buf.split(b":", 1)
		length = int(length)
		while len(rest) < length:
			rest += self.sock.recv(65536)
		self.buf = rest[length:]
		return json.loads(rest[:length])

	def cmd(self, name, params=None):
		self.id += 1
		data = json.dumps([0, self.id, name, params or {}]).encode()
		self.sock.sendall(str(len(data)).encode() + b":" + data)
		while True:
			msg = self.read()
			if msg[0] == 1 and msg[1] == self.id:
				if msg[2]:
					raise RuntimeError(msg[2])
				return msg[3]

	def js(self, script):
		return self.cmd("WebDriver:ExecuteScript", {"script": "let w = window.wrappedJSObject || window; " + script, "args": []})["value"]


def wait(m, script, timeout=20):
	end = time.time() + timeout
	value = None
	while time.time() < end:
		try:
			value = m.js(script)
		except RuntimeError as e:
			value = str(e)
		if value is True:
			return True
		time.sleep(0.5)
	print("  timeout, last value:", value)
	return False


def item(device, prop, name):
	return "let p = w.INDIGO.findProperty('%s', '%s'); return p != null && p.item('%s') != null && p.item('%s').value == true;" % (device, prop, name, name)


m = Marionette()
m.cmd("WebDriver:NewSession", {})
m.cmd("WebDriver:SetWindowRect", {"width": 1400, "height": 1100})
results = []

m.cmd("WebDriver:Navigate", {"url": BASE + "astrometry.html"})
results.append(("astrometry: solver agent properties visible", wait(m, "return w.INDIGO != null && w.INDIGO.findProperty('Platesolver Agent', 'AGENT_START_PROCESS') != null;")))
results.append(("astrometry: solverAgent() is Platesolver Agent", wait(m, "return w.solverAgent() === 'Platesolver Agent';")))
results.append(("astrometry: no waiting banner", wait(m, "return document.body.innerText.indexOf('Waiting for') < 0;")))
results.append(("astrometry: command buttons rendered", wait(m, "return w.astrometryStartProcessHasItem('SOLVE') && document.querySelectorAll('.indigo-command-row button').length >= 6;")))
results.append(("astrometry: related Imager Agent selected", wait(m, item("Platesolver Agent", "FILTER_RELATED_AGENT_LIST", "Imager Agent"))))
results.append(("astrometry: related Mount Agent selected", wait(m, item("Platesolver Agent", "FILTER_RELATED_AGENT_LIST", "Mount Agent"))))
time.sleep(1)
open(OUT + "/m-astrometry.png", "wb").write(base64.b64decode(m.cmd("WebDriver:TakeScreenshot", {"full": True})["value"]))

m.cmd("WebDriver:Navigate", {"url": BASE + "imager.html"})
results.append(("imager: related Platesolver Agent selected", wait(m, "return w.INDIGO != null && (function(){" + item("Imager Agent", "FILTER_RELATED_AGENT_LIST", "Platesolver Agent") + "})();")))

m.cmd("WebDriver:Navigate", {"url": BASE + "mount.html"})
results.append(("mount: related Platesolver Agent selected", wait(m, "return w.INDIGO != null && (function(){" + item("Mount Agent", "FILTER_RELATED_AGENT_LIST", "Platesolver Agent") + "})();")))

m.cmd("WebDriver:DeleteSession", {})
for name, ok in results:
	print(("PASS " if ok else "FAIL ") + name)
