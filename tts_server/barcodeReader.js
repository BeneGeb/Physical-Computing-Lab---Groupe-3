const { exec } = require('child_process');
const fs = require('fs');

function readBarcodeFromImage(imagePath) {
  return new Promise((resolve, reject) => {
    exec(`zbarimg --quiet --nodbus --oneshot --raw ${imagePath}`, (error, stdout, stderr) => {
      if (error) {
        return reject(new Error(`Barcode read error: ${stderr.trim() || error.message}`));
      }

      const barcode = stdout.trim();
      if (!barcode) {
        return reject(new Error('No barcode found'));
      }

      resolve(barcode);
    });
  });
}

module.exports = { readBarcodeFromImage };