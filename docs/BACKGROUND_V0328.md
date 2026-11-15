# Fond RGB565 memoire maitrisee

DGB1 + largeur uint16 LE + hauteur uint16 LE + pixels RGB565 LE. Le plugin produit un quart de largeur et de hauteur.

Le firmware accepte au plus 96 Kio en upload multipart (LittleFS, fichier temporaire) ; l'image est chargee dans un unique tampon lors du rendu de la page et liberee au changement de page. Sinon, couleur unie et message [BACKGROUND] sur moniteur série.

Risque restant : dependance aux capacites LVGL 8/9 et RAM contigue; validation materielle obligatoire, particulierement les cartes sans PSRAM. Ne pas ecraser la version precedente avant validation.
