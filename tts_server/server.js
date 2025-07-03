const express = require('express');
const axios = require('axios');
const dotenv = require('dotenv');
const multer = require('multer');
const { exec } = require('child_process');
const path = require('path');
const fs = require('fs');

const { fetchProductFromBarcode } = require('./foodAPI');
const { readBarcodeFromImage } = require('./barcodeReader');

dotenv.config();

const app = express();
const upload = multer({ dest: 'uploads/' });
app.use(express.json({ limit: '10mb' }));
const PORT = 5000;
const OPENAI_API_KEY = process.env.OPENAI_API_KEY;

app.get('/', (req, res) => {
  res.send('TTS Proxy Server is running');
});

app.get('/tts', async (req, res) => {
  console.log('Neue Request!');
  console.log(req.query);
  const text = req.query.text;
  const voice = req.query.voice || 'nova';

  if (!text || !OPENAI_API_KEY) {
    return res.status(400).send('Missing "text" query or API key.');
  }

  try {
    const response = await axios.post(
      'https://api.openai.com/v1/audio/speech',
      {
        model: 'tts-1',
        input: text,
        voice: voice,
        response_format: 'mp3'
      },
      {
        responseType: 'stream',
        headers: {
          Authorization: `Bearer ${OPENAI_API_KEY}`,
          'Content-Type': 'application/json'
        }
      }
    );

    res.setHeader('Content-Type', 'audio/mpeg');
    response.data.pipe(res);
  } catch (error) {
    console.error('TTS request failed:', error.response?.data || error.message);
    res.status(500).send('TTS proxy error');
  }
});

function saveBase64Image(base64Image) {
  return new Promise((resolve, reject) => {
    try {
      // Assume jpeg for now
      const base64Data = base64Image.replace(/^data:image\/jpeg;base64,/, '');
      const filename = path.join(__dirname, 'uploads', `temp_${Date.now()}.jpg`);
      fs.writeFile(filename, Buffer.from(base64Data, 'base64'), (err) => {
        if (err) return reject(err);
        resolve(filename);
      });
    } catch (err) {
      reject(err);
    }
  });
}

async function processScan(imageBase64) {
  if (!imageBase64) throw new Error('No image data provided');

  const filename = await saveBase64Image(imageBase64);
  try {
    const barcode = await readBarcodeFromImage(filename);
    if (!barcode) throw new Error('No barcode found in image');

    const product = await fetchProductFromBarcode(barcode);
    if (!product) throw new Error('Product not found in Open Food Facts');

    return { barcode, product };
  } finally {
    fs.unlink(filename, err => {
      if (err) console.error('Failed to delete temp file:', err.message);
    });
  }
}

app.post('/scan', async (req, res) => {
  try {
    const result = await processScan(req.body.image);
    res.json(result);
  } catch (err) {
    res.status(400).json({ error: err.message });
  }
});

app.listen(PORT, () => {
  console.log(`TTS proxy listening on port ${PORT}`);
});
