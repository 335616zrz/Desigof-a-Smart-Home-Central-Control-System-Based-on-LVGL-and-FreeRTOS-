'use strict';
// Generate solid/gradient PNG icons (192, 512) without external deps
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

function crcTable(){
  const table = new Uint32Array(256);
  for(let n=0;n<256;n++){
    let c=n;
    for(let k=0;k<8;k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
    table[n]=c>>>0;
  }
  return table;
}
const CRC_TABLE = crcTable();
function crc32(buf){
  let c=0xFFFFFFFF;
  for(let i=0;i<buf.length;i++) c = CRC_TABLE[(c ^ buf[i]) & 0xFF] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

function u32(n){ const b=Buffer.alloc(4); b.writeUInt32BE(n>>>0,0); return b; }

function chunk(type, data){
  const t = Buffer.from(type, 'ascii');
  const len = u32(data.length);
  const crc = u32(crc32(Buffer.concat([t, data])));
  return Buffer.concat([len, t, data, crc]);
}

function pngEncode(w,h, rgbaProvider){
  const SIGN = Buffer.from([137,80,78,71,13,10,26,10]);
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(w,0); ihdr.writeUInt32BE(h,4);
  ihdr[8]=8; // bit depth
  ihdr[9]=6; // color type RGBA
  ihdr[10]=0; ihdr[11]=0; ihdr[12]=0;

  const stride = w*4 + 1; // filter byte + pixels
  const raw = Buffer.alloc(stride*h);
  for(let y=0;y<h;y++){
    const row = raw.subarray(y*stride, (y+1)*stride);
    row[0]=0; // filter 0
    for(let x=0;x<w;x++){
      const idx = 1 + x*4;
      const rgba = rgbaProvider(x,y,w,h);
      row[idx+0]=rgba[0];
      row[idx+1]=rgba[1];
      row[idx+2]=rgba[2];
      row[idx+3]=rgba[3];
    }
  }
  const idat = zlib.deflateSync(raw);
  return Buffer.concat([SIGN, chunk('IHDR', ihdr), chunk('IDAT', idat), chunk('IEND', Buffer.alloc(0))]);
}

function lerp(a,b,t){ return a+(b-a)*t; }
function colorHexToRGBA(hex){
  const h = hex.replace('#','');
  const r = parseInt(h.slice(0,2),16);
  const g = parseInt(h.slice(2,4),16);
  const b = parseInt(h.slice(4,6),16);
  return [r,g,b,255];
}

function gradientMaker(top,bottom){
  const c1 = colorHexToRGBA(top);
  const c2 = colorHexToRGBA(bottom);
  return (x,y,w,h)=>{
    const t = y/(h-1);
    return [
      Math.round(lerp(c1[0],c2[0],t)),
      Math.round(lerp(c1[1],c2[1],t)),
      Math.round(lerp(c1[2],c2[2],t)),
      255
    ];
  };
}

function ensureDir(p){ fs.mkdirSync(p, { recursive: true }); }

const outDir = path.join(__dirname, '..', 'public', 'assets');
ensureDir(outDir);

const icon192 = pngEncode(192,192, gradientMaker('#58b7ff', '#0b1220'));
fs.writeFileSync(path.join(outDir,'icon-192.png'), icon192);
const icon512 = pngEncode(512,512, gradientMaker('#58b7ff', '#0b1220'));
fs.writeFileSync(path.join(outDir,'icon-512.png'), icon512);
// Apple touch icon 180
const icon180 = pngEncode(180,180, gradientMaker('#58b7ff', '#0b1220'));
fs.writeFileSync(path.join(outDir,'apple-touch-icon.png'), icon180);

console.log('Generated icons: 192, 512, 180');

