const USER_KEY = "username";
const SECRET_KEY = "totpsecret";
const TIMESTEP_SEC = 15;

export function getSavedCreds() {
  const username = localStorage.getItem(USER_KEY);
  const secretHex = localStorage.getItem(SECRET_KEY);
  if (!username || !secretHex) return null;
  return { username, secretHex };
}

export function setSavedCreds(username, secretHex) {
  localStorage.setItem(USER_KEY, username);
  localStorage.setItem(SECRET_KEY, secretHex);
}

export function clearSavedCreds() {
  localStorage.removeItem(USER_KEY);
  localStorage.removeItem(SECRET_KEY);
}

export function isHexEven(str) {
  return typeof str === "string" && str.length > 0 && (str.length % 2 === 0) && /^[0-9a-fA-F]+$/.test(str);
}

export async function sha256Hex(message) {
  const data = new TextEncoder().encode(message);
  const hash = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(hash)).map(b => b.toString(16).padStart(2, "0")).join("");
}

export function hexToBytes(hex) {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.substr(i * 2, 2), 16);
  return out;
}

export async function generateTOTP(secretHex, counter) {
  const keyBytes = hexToBytes(secretHex);

  const counterBuf = new ArrayBuffer(8);
  const view = new DataView(counterBuf);
  view.setBigUint64(0, BigInt(counter), false); // big-endian
  const key = await crypto.subtle.importKey(
    "raw",
    keyBytes,
    { name: "HMAC", hash: "SHA-1" },
    false,
    ["sign"]
  );
  const sig = await crypto.subtle.sign("HMAC", key, counterBuf);
  const hmac = new Uint8Array(sig);
  const offset = hmac[hmac.length - 1] & 0x0f;
  const binary =
    ((hmac[offset] & 0x7f) << 24) |
    ((hmac[offset + 1] & 0xff) << 16) |
    ((hmac[offset + 2] & 0xff) << 8) |
    (hmac[offset + 3] & 0xff);

  return (binary % 1000000).toString().padStart(6, "0");
}

export function makeSalt() {
  return Math.random().toString(36).slice(2, 12);
}

// `${sha256(username + totp + salt)}?${salt}`
export async function makeAuthHeaderFrom(username, secretHex) {
  const salt = makeSalt();
  const counter = Math.floor(Date.now() / 1000 / TIMESTEP_SEC);
  const totp = await generateTOTP(secretHex, counter);
  const hash = await sha256Hex(username + totp + salt);
  return `${hash}?${salt}`;
}

export async function makeAuthHeader() {
  const creds = getSavedCreds();
  if (!creds) return null;
  return makeAuthHeaderFrom(creds.username, creds.secretHex);
}

export async function api(url, options = {}, { loginUrl = "login.html" } = {}) {
  const headers = { ...(options.headers || {}) };
  const auth = await makeAuthHeader();
  if (auth) headers.Authorization = auth;
  const res = await fetch(url, { ...options, headers });
  const ct = (res.headers.get("content-type") || "").toLowerCase();
  if (ct.includes("application/json")) return await res.json();
  const text = await res.text();
  if (text.includes("PluginCore Login")) {
    window.location.href = loginUrl;
    return null;
  }
  return text;
}
