import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const output = resolve(scriptDir, "../../Content/JavaScript/node_modules/@matrix/puerts-runtime");

mkdirSync(output, { recursive: true });
writeFileSync(resolve(output, "package.json"), JSON.stringify({
  name: "@matrix/puerts-runtime",
  version: "1.0.0",
  main: "index.js"
}, null, 2) + "\n", "utf8");
