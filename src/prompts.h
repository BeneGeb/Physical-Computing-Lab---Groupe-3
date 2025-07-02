#ifndef PROMPTS_H
#define PROMPTS_H

// --- Prompts for ChatGPT ---
const char *PROMPT_IMAGE_RECOGNITION = "Gebe in nur 1-2 Wörtern zurück, um was für ein Lebensmittel es sich auf dem Bild handelt. Durchsuche dafür die mitgeschickte Liste und gucke ob sich ein passendes Lebensmittel bereits darin befindet. Falls ja, gib dieses zurück. Falls nein, nenne das Lebensmittel so wie es auf dem Bild zu sehen ist. Achte darauf, dass du nur ein Wort zurückgibst, auch wenn das Lebensmittel mehrere Wörter hat. Wenn du dir nicht sicher bist, gib 'unbekannt' zurück.";
const char *PROMPT_ADD_PRODUCT = "(Trenne alle Wörter in deiner Antwort bitte ausschließlich mit Leerzeichen. Verwende keine Tabulatoren, Zeilenumbrüche, Bindestriche etc.) Sage in einem kurzen Satz, dass folgendes Produkt in den Kühlschrank gelegt wurde: ";
const char *PROMPT_REMOVE_PRODUCT = "(Trenne alle Wörter in deiner Antwort bitte ausschließlich mit Leerzeichen. Verwende keine Tabulatoren, Zeilenumbrüche, Bindestriche etc.) Sage in einem kurzen Satz, dass folgendes Produkt aus dem Kühlschrank genommen wurde: ";

#endif // PROMPTS_H