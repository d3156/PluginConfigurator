import resolve from "@rollup/plugin-node-resolve";
import commonjs from "@rollup/plugin-commonjs";

export default {
  input: "editor.js",
  output: {
    file: "../web/editor.bundle.js",
    format: "iife",
    name: "PCEditor"   // window.PCEditor.createJsonEditor(...)
  },
  plugins: [resolve(), commonjs()]
};
