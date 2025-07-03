const axios = require('axios');

// The answer from Open Food Facts API is quite large,
// so we extract only the necessary fields
function extractProductData(data) {
  const p = data.product;

  return {
    code: data.code,
    name: p.product_name,
    brand: p.brands,
    quantity: p.quantity,
    categories: p.categories_tags?.slice(0, 3),
    ingredients: p.ingredients_text,
    allergens: p.allergens_tags,
    nutriscore: p.nutriscore_grade,
    nova_group: p.nova_group,
    ecoscore: p.ecoscore_grade,
    image: p.image_front_url,
  };
}

// Fetch product information from Open Food Facts using a barcode
async function fetchProductFromBarcode(barcode) {
  try {
    const url = `https://world.openfoodfacts.org/api/v0/product/${barcode}.json`;
    const response = await axios.get(url);

    if (response.data.status === 0) {
      // Barcode not found
      return null;
    }


    // return only the necessary product data
    return extractProductData(response.data);
  } catch (error) {
    console.error(`Error fetching product for barcode ${barcode}:`, error.message);
    throw error;
  }
}

module.exports = { fetchProductFromBarcode };