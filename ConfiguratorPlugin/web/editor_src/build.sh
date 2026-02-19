#!/usr/bin/env bash

sudo apt install npm
npm init -y
npm i codemirror @codemirror/lang-json
npm i -D rollup @rollup/plugin-node-resolve @rollup/plugin-commonjs
npx rollup -c
